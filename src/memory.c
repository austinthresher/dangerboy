#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "debugger.h"
#include "memory.h"
#include "ppu.h"

bool break_on_invalid = false;
word dma_src, dma_dst, dma_rst;
void mem_dma(byte val);

void mem_init(void) {
   mem_free();
   dma_dst              = 0;
   dma_src              = 0;
   dma_rst              = 0;
   mem_mbc_type         = NONE;
   mem_ram_bank_count   = 0;
   mem_rom_bank_count   = 2;
   mem_current_ram_bank = 1;
   mem_current_rom_bank = 0;
   mem_mbc_bankmode     = ROM16_RAM8;
   mem_ram_bank_locked  = true;
   mem_buttons          = 0x0F;
   mem_dpad             = 0x0F;
   mem_input_last_write = 0;
   mem_oam_state            = INACTIVE;
   mem_ram      = (byte*)calloc(0x10000, 1);
   mem_ram_bank = (byte*)calloc(0x10000, 1);
}

void mem_free() {
   if (mem_ram != NULL) {
      free(mem_ram);
   }
   mem_ram = NULL;

   if (mem_ram_bank != NULL) {
      free(mem_ram_bank);
   }
   mem_ram_bank = NULL;

   if (mem_rom != NULL) {
      free(mem_rom);
   }
   mem_rom = NULL;
}

void mem_load_image(char* fname) {
   assert(mem_ram != NULL);
   FILE* fin = fopen(fname, "rb");
   if (fin == NULL) {
      fprintf(stderr, "Could not open file %s\n", fname);
      mem_free();
      exit(1);
   }

   // Load the first 32kb of the ROM to RAM
   uint32_t i = 0;
   while (!feof(fin) && i < 0x8000) {
      byte data         = 0;
      size_t bytes_read = fread(&data, 1, 1, fin);
      if (bytes_read != 1) {
         fprintf(stderr, "Error reading %s\n", fname);
         fclose(fin);
         mem_free();
         exit(1);
      }
      mem_ram[i] = data;
      i++;
   }

   int rom_size = mem_direct_read(0x0148);
   if (rom_size < 8) {
      mem_rom_bank_count = pow(2, rom_size+1);
      rom_size = mem_rom_bank_count * 0x4000;
      mem_rom = calloc(rom_size, 1);
   } else {
      fprintf(stderr, "Unsupported bank configuration: %02X\n", rom_size);
      mem_free();
      exit(1);
   }

   // Store the entire cart in ROM so we can bank
   fseek(fin, 0, SEEK_SET);
   i = 0;
   while (!feof(fin) && i < rom_size) {
      byte data;
      size_t bytes_read = fread(&data, 1, 1, fin);
      if (bytes_read != 1) {
         fprintf(stderr, "Error reading %s\n", fname);
         fclose(fin);
         mem_free();
         exit(1);
      }
      mem_rom[i] = data;
      i++;
   }

   fclose(fin);

   // Determine cart type. Fallthrough is intentional.
   switch (mem_ram[CART_TYPE_ADDR]) {
      case 0x00:
         // ROM Only
         mem_mbc_type = NONE;
         break;
      case 0x03:
      // ROM + MBC1 + RAM + BATT
      case 0x02:
      // ROM + MBC1 + RAM
      case 0x01:
         // ROM + MBC1
         mem_mbc_type = MBC1;
         break;
      case 0x06:
      // ROM + MBC2 + BATTERY
      case 0x05:
         // ROM + MBC2
         mem_mbc_type = MBC2;
         break;
      case 0x0F:
      // ROM + MBC3 + TIMER + BATT
      case 0x10:
      // ROM + MBC3 + TIMER + RAM + BATT
      case 0x11:
      // ROM + MBC3
      case 0x12:
      // ROM + MBC3 + RAM
      case 0x13:
         // ROM + MBC3 + RAM + BATT
         mem_mbc_type = MBC3;
         break;
      // MBC5 currently unsupported
      default:
         fprintf(stderr, "Unknown banking mode: %X", mem_ram[CART_TYPE_ADDR]);
         exit(1);
         break;
   }

   // RAM Size
   switch (mem_ram[RAM_SIZE_ADDR]) {
      case 0: mem_ram_bank_count = 0; break;
      case 1:
      case 2: mem_ram_bank_count = 1; break;
      case 3: mem_ram_bank_count = 4; break;
      case 4: mem_ram_bank_count = 16; break;
      default:
         fprintf(stderr, "Unknown RAM bank count: %X", mem_ram[RAM_SIZE_ADDR]);
         mem_free();
         exit(1);
         break;
   }

   mem_current_rom_bank = 1;
   mem_current_ram_bank = 0;

   // 16 bytes at ROM_NAME_ADDR contain game title in upper case
   memcpy(mem_rom_name, mem_ram + ROM_NAME_ADDR, 14);
   mem_rom_name[15] = '\0';

}

void mem_print_rom_info() {
   printf("Name:\t\t%s\n", mem_rom_name);
   printf("MBC:\t\t%d\n", mem_mbc_type);
   printf("ROM Banks:\t%d\n", mem_rom_bank_count);
   printf("RAM Banks:\t%d\n", mem_ram_bank_count);
}

void mem_dma(byte val) {
   if (mem_oam_state == INACTIVE) {
      debugger_log("OAM DMA Starting");
      mem_oam_state = STARTING;
      dma_src = val << 8;
      dma_dst = SPRITE_RAM_START_ADDR;;
   } else {
      debugger_log("OAM DMA Restarting");
      mem_oam_state = RESTARTING;
      dma_rst = val << 8;
   }
}

void mem_advance_time(tick ticks) {
   if (mem_oam_state != INACTIVE) {
      while (ticks > 0) {
         ticks -= 4;
         if (mem_oam_state == STARTING) {
            debugger_log("OAM DMA Started");
            mem_oam_state = ACTIVE;
            continue;
         }

         mem_direct_write(dma_dst, mem_rb(dma_src));

         dma_src++;
         dma_dst++;

         if (dma_dst >= SPRITE_RAM_END_ADDR+1
          && mem_oam_state != RESTARTING) {
            dma_dst = 0;
            dma_src = 0;
            debugger_log("OAM DMA Finish");
            mem_oam_state = INACTIVE;
            break;
         }

         if (mem_oam_state == RESTARTING) {
            debugger_log("OAM DMA Restarted");
            mem_oam_state = ACTIVE;
            dma_src = dma_rst;
            dma_dst = SPRITE_RAM_START_ADDR;
            continue;
         }
      }
   }
}

byte mem_get_current_rom_bank() {
   if (mem_mbc_type == MBC1) {
      // In ROM16_RAM8 mode, mem_current_ram_bank
      // holds bits 5-6 of our ROM bank index.
      if (mem_mbc_bankmode == ROM16_RAM8) {
         byte bank = (mem_current_rom_bank & 0x1F) | ((mem_current_ram_bank & 0x3) << 5);
         return bank;
      } else {
         byte bank = mem_current_rom_bank & 0x1F;
         return bank;
      }
   }
   // TODO: Make sure MBC2 doesn't need special behavior here
   // Other MBCs
   if ((mem_current_rom_bank & 0x7F) == 0) {
      return 1;
   }
   return mem_current_rom_bank & 0x7F;
}

void mem_direct_write(word addr, byte val) { mem_ram[addr] = val; }
byte mem_direct_read(word addr) { return mem_ram[addr]; }

void mem_wb(word addr, byte val) {
   debugger_notify_mem_write(addr, val);

   switch (addr & 0xF000) {
      case 0x0000: // External ram enable / disable
      case 0x1000: 
         if (mem_mbc_type != NONE) {
            // MBC2 has the restriction that the least significant
            // bit of the upper address byte must be zero
            if (mem_mbc_type != MBC2 || (addr & 0x100) == 0) {

               // Writing to this region of memory enables or
               // disables external RAM, based on the value
               if ((val & 0x0F) == 0x0A) {
                  mem_ram_bank_locked = false;
               } else {
                  mem_ram_bank_locked = true;
               }
            }
         }
         return;
      case 0x2000: // ROM bank select
      case 0x3000:
         if (mem_mbc_type == NONE) {
            return;
         }
         if (mem_mbc_type == MBC1) {
            // Writing to this region selects the lower 5 bits
            // of the ROM bank index
            val &= 0x1F;
            if (val == 0) {
               val = 1;
            }
            mem_current_rom_bank = val;
            return;
         }
         if (mem_mbc_type == MBC2) {
            // For MBC2, the least significant bit of the upper
            // address byte must be one
            if (addr & 0x0100) {
               val &= 0x0F;
               if (val == 0) {
                  val = 1;
               }
               mem_current_rom_bank = val;
            }
            return;
         }
         if (mem_mbc_type == MBC3) {
            // MBC3 uses all 7 bits here instead of splitting
            // into hi / lo writes
            val &= 0x7F;
            mem_current_rom_bank = val;
            return;
         }
         return;
      case 0x4000: // RAM bank select
      case 0x5000:
         if (mem_mbc_type != NONE) {
            // This either selects our RAM bank for ROM4_RAM32
            // bank mode, or bits 5-6 of our ROM for ROM16_RAM8
            if (mem_mbc_type != MBC3 || val < 0x4) {
               mem_current_ram_bank = val & 0x03;
            } else {
               // TODO: MBC3 can also map real time
               // clock registers by writing here
            }
         }
         return;
      case 0x6000: // Bank mode and RTC
      case 0x7000:
         // This register selects our ROM / RAM banking mode
         // for MBC1 and MBC2. MBC3 uses this location to
         // latch current time into RTC registers (TODO: That.)
         if (mem_mbc_type == MBC1 || mem_mbc_type == MBC2) {
            if ((val & 0x01) == 0) {
               mem_mbc_bankmode = ROM16_RAM8;
            }
            if ((val & 0x01) == 1) {
               mem_mbc_bankmode = ROM4_RAM32;
            }
         }
         return;
      case 0x8000: // VRAM
      case 0x9000:
         if ((mem_ram[LCD_STATUS_ADDR] & 3) != PPU_MODE_SCAN_VRAM
          || lcd_disable) {
            mem_ram[addr] = val;
         }
         return;
      case 0xA000: // External RAM
      case 0xB000:
         if (mem_mbc_type == NONE) {
            mem_ram[addr] = val;
            return;
         }
         if (mem_ram_bank_locked == false) {
            addr -= 0xA000;
            if (mem_mbc_type == MBC3 || mem_mbc_bankmode == ROM4_RAM32) {
               mem_ram_bank[
                  (addr + mem_current_ram_bank * 0x2000) & 0xFFFF] = val;
               return;
            } 
            mem_ram_bank[addr] = val;
         }
         return;
      case 0xC000: // Work RAM
      case 0xD000:
         mem_ram[addr] = val;
         return;
      case 0xE000: // 0xC000 mirror
         mem_ram[addr - 0x2000] = val;
         return;
      case 0xF000:
         if (addr < 0xFE00) { // Partial 0xD000 mirror
            mem_ram[addr - 0x2000] = val;
            return;
         }
         if (addr < 0xFEA0) { // FE00 to FE9F is OAM
            // OAM can only be written to during HBLANK or VBLANK,
            // and not during an OAM DMA transfer
            if ((mem_ram[LCD_STATUS_ADDR] & 3) == PPU_MODE_HBLANK
             || (mem_ram[LCD_STATUS_ADDR] & 3) == PPU_MODE_VBLANK
             || lcd_disable) {
               if (mem_oam_state == INACTIVE
                || mem_oam_state == STARTING) {
                  mem_ram[addr] = val;
               }
            }
            return;
         }
         break;
      default: break;
   }
   // We will only reach here if our address wasn't handled
   // in the 0xF000 case. We can assume we're dealing with
   // HRAM or hardware registers.
   if (addr >= 0xFEA0 && addr < 0xFEFF) {
      return; // This memory is not usable
   }

   // HW Registers
   switch (addr) {
      case DIV_REGISTER_ADDR:
         // TIMA and DIV use the same internal counter,
         // so resetting DIV also resets TIMA
         cpu_reset_timer();
         mem_ram[DIV_REGISTER_ADDR] = 0;
         return;
      case LCD_CONTROL_ADDR:
      case LCD_STATUS_ADDR:
      case LCD_SCY_ADDR:
      case LCD_SCX_ADDR:
      case LCD_LINE_Y_ADDR:
      case LCD_LINE_Y_C_ADDR:
         ppu_update_register(addr, val);
         return;
      case OAM_DMA_ADDR:
         mem_dma(val);
         return;
      case INPUT_REGISTER_ADDR:
         mem_input_last_write = val & 0x30;
         return;
      default: break;
   }

   //0xFF00 to 0xFFFF
   mem_ram[addr] = val;
}

byte mem_rb(word addr) {
   debugger_notify_mem_read(addr);
   
   switch(addr & 0xF000) {
      case 0x0000: // ROM bank 0
      case 0x1000:
      case 0x2000:
      case 0x3000: 
         return mem_rom[addr];
      case 0x4000: // Rom banks 1-X
      case 0x5000:
      case 0x6000:
      case 0x7000:
         if (mem_mbc_type == NONE) {
            return mem_rom[addr];
         }
         addr -= 0x4000;
         return mem_rom[
            (mem_get_current_rom_bank()
             % mem_rom_bank_count)
            * 0x4000
            + addr];
      case 0x8000: // VRAM
      case 0x9000: 
         if ((mem_ram[LCD_STATUS_ADDR] & 3) != PPU_MODE_SCAN_VRAM
          || lcd_disable) {
            return mem_ram[addr];
         }
         return 0xFF;
      case 0xA000: // External RAM
      case 0xB000: 
         if (mem_mbc_type == NONE) {
            return mem_ram[addr];
         }
         addr -= 0xA000;

         if (mem_mbc_type == MBC3 || mem_mbc_bankmode == ROM4_RAM32) {
            return mem_ram_bank[
                (addr + mem_current_ram_bank * 2000) & 0xFFFF];
         }
         return mem_ram_bank[addr];
      case 0xC000:
      case 0xD000: // Work RAM
         return mem_ram[addr];
      case 0xE000: // Mirror of 0xC000
         return mem_ram[addr - 0x2000]; 
      case 0xF000:
         if (addr < 0xFE00) { // Partial 0xD000 mirror
            return mem_ram[addr - 0x2000];
         }
         if (addr < 0xFEA0) { // FE00 to FE9F is OAM
            // OAM can only be read during HBLANK or VBLANK,
            // and not during an OAM DMA transfer
            if ((mem_ram[LCD_STATUS_ADDR] & 3) == PPU_MODE_HBLANK
             || (mem_ram[LCD_STATUS_ADDR] & 3) == PPU_MODE_VBLANK
             || lcd_disable) {
               if (mem_oam_state == INACTIVE
                || mem_oam_state == STARTING) {
                  return mem_ram[addr];
               }
            }
            return 0xFF;
         }
         break;
      default: break;
   }

   // We will only reach here if our address wasn't handled
   // in the 0xF000 case. We can assume we're dealing with
   // HRAM or hardware registers.

   if (addr >= 0xFEA0 && addr < 0xFEFF) {
      return 0xFF; // This memory is not usable
   }

   // Hardware registers
   switch (addr) {
      case 0xFF00: // Input register
         if (mem_input_last_write == 0x00) {
            return 0xC0 | (mem_dpad & mem_buttons);
         }
         if (mem_input_last_write == 0x10) {
            return 0xC0 | mem_buttons;
         }
         if (mem_input_last_write == 0x20) {
            return 0xC0 | mem_dpad;
         }
         return 0xFF;
      case INT_ENABLED_ADDR:
         return 0xE0 | (mem_ram[INT_ENABLED_ADDR] & 0x1F);
      case INT_FLAG_ADDR:
         return 0xE0 | (mem_ram[INT_FLAG_ADDR] & 0x1F);
      default: break;
   }

   // HRAM and other registers
   return mem_ram[addr]; 
}

void mem_ww(word addr, word val) // Write word
{
   mem_wb(addr, val & 0x00FF);
   mem_wb(addr + 1, (val & 0xFF00) >> 8);
}

word mem_rw(word addr) {
   word result = 0;
   result += mem_rb(addr);
   result += mem_rb(addr + 1) << 8;
   return result;
}
