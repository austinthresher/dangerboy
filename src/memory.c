#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "debugger.h"
#include "memory.h"
#include "ppu.h"

void mem_dma(byte val);

void mem_init(void) {
   mem_free();
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
   word dma = val << 8;
   word ad  = SPRITE_RAM_START_ADDR;
   while (ad <= SPRITE_RAM_END_ADDR) {
      mem_ram[ad++] = mem_rb(dma++);
   }
}

// Use these to implement system read writes, like OAM transfer
void mem_direct_write(word addr, byte val) { mem_ram[addr] = val; }

byte mem_direct_read(word addr) { return mem_ram[addr]; }

// Write byte
void mem_wb(word addr, byte val) {

   debugger_notify_mem_write(addr, val);

   if (addr < 0x8000) {
      if (mem_mbc_type == NONE) {
         // Nothing under 0x8000 is writable without banking
         return;
      }
      if (addr < 0x2000) {
         if (mem_mbc_type >= MBC1) {

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
      } else if (addr < 0x4000) {
         if (mem_mbc_type == MBC1) {

            // Writing to this region selects the lower 5 bits
            // of the ROM bank index
            val &= 0x1F;
            if (val == 0) {
               val = 1;
            }
            mem_current_rom_bank = val;
         } else if (mem_mbc_type == MBC2) {

            // For MBC2, the least significant bit of the upper
            // address byte must be one
            if (addr & 0x0100) {
               val &= 0x0F;
               mem_current_rom_bank = val;
            }
         } else if (mem_mbc_type == MBC3) {

            // MBC3 uses all 7 bits here instead of splitting
            // into hi / lo writes
            val &= 0x7F;
            mem_current_rom_bank = val;
         }
      } else if (addr < 0x6000) {
         if (mem_mbc_type >= MBC1) {
            // This either selects our RAM bank for ROM4_RAM32
            // bank mode, or bits 5-6 of our ROM for ROM16_RAM8
            if (mem_mbc_type != MBC3 || val < 0x4) {
               mem_current_ram_bank = val & 0x03;
            } else {
               // TODO: MBC3 can also map real time
               // clock registers by writing here
            }
         }
      } else if (addr < 0x8000) {
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
      }
   } else if (addr > 0x8000 && addr < 0xA000) {
      // This memory is only accessible during hblank or vblank.
      byte mode = mem_ram[LCD_STATUS_ADDR] & 0x03;
      // If we're in hblank / vblank or lcd is disabled
      if (mode == 0 || mode == 1 || lcd_disable) {
         mem_ram[addr] = val;
      }
   } else if (addr >= 0xA000 && addr < 0xC000) {
      // Maybe need to check if banking is enabled here?
      if (mem_mbc_type == NONE) {
         mem_ram[addr] = val;
      } else {
         if (mem_ram_bank_locked == false) {
            addr -= 0xA000;
            addr &= 0x1FFF;
            if (mem_mbc_bankmode == ROM16_RAM8) {
               mem_ram_bank[addr] = val;
            } else {
               word bank_addr = (addr + mem_current_ram_bank * 0x2000) & 0xFFFF;
               mem_ram_bank[bank_addr] = val;
            }
         }
      }
   } else if (addr >= SPRITE_RAM_START_ADDR && addr <= SPRITE_RAM_END_ADDR) {
      // This memory is only accessible during hblank or vblank.
      byte mode = mem_ram[LCD_STATUS_ADDR] & 0x03;
      // If we're in hblank / vblank or lcd is disabled
      if (mode == 0 || mode == 1 || (mem_ram[LCD_CONTROL_ADDR] & 0x80) == 0) {
         mem_ram[addr] = val;
      }
   } else {
      // Hardware registers
      switch (addr) {
         case DIV_REGISTER_ADDR:
            mem_ram[DIV_REGISTER_ADDR] = 0;
            break;
         case LCD_CONTROL_ADDR:
         case LCD_STATUS_ADDR:
         case LCD_SCY_ADDR:
         case LCD_SCX_ADDR:
         case LCD_LINE_Y_ADDR:
         case LCD_LINE_Y_C_ADDR: ppu_update_register(addr, val); break;
         case OAM_DMA_ADDR: mem_dma(val); break;
         case INPUT_REGISTER_ADDR: mem_input_last_write = val & 0x30; break;
         default:
            if (addr >= 0xE000 && addr <= 0xFE00) {
               addr -= 0x2000; // Mirrored memory
            }
            mem_ram[addr] = val;
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

// Read byte
byte mem_rb(word addr) {

   debugger_notify_mem_read(addr);

   // 0x0000 to 0x3FFF always contains the first 16 kb of ROM
   if (addr < 0x4000) {
      return mem_ram[addr];
   }
   
   if (mem_mbc_type == NONE && addr < 0x8000) {
      return mem_ram[addr];
   }

   // 0x4000 to 0x7FFF can contain any other ROM bank
   if (mem_mbc_type != NONE && addr >= 0x4000 && addr < 0x8000) {
      addr -= 0x4000;
      addr &= 0x3FFF;
      byte bank = mem_get_current_rom_bank();
      if (bank > mem_rom_bank_count) {
         fprintf(stderr, "Tried to access rom bank %d out of %d\n",
               bank, mem_rom_bank_count);
         exit(1);
      }
      return mem_rom[bank * 0x4000 + addr];
   }

   // 0xA000 to 0xBFFF is used for external RAM, usually SRAM
   // TODO: Make sure this works with MBC2, which has 512x4 bits
   // of "external" RAM. The full address space is not used there.
   if (mem_mbc_type != NONE && addr >= 0xA000 && addr < 0xC000) {
      
      // The external RAM could be up to 2kb, 8kb, or 32kb
      // 32kb mode required 4 RAM banks
      addr -= 0xA000;
      if (mem_mbc_type < MBC3 || mem_mbc_bankmode != ROM4_RAM32) {
         return mem_ram_bank[addr];
      }
      return mem_ram_bank[mem_current_ram_bank * 0x2000 + addr];
   }

   if (addr == 0xFF00) {
      switch (mem_input_last_write) {
         case 0x00: return mem_dpad & mem_buttons;
         case 0x10: return mem_buttons;
         case 0x20: return mem_dpad;
         default: return 0x00;
      }
   }

   if ((addr > 0x8000 && addr < 0xA000) || (addr > 0xFE00 && addr < 0xFE9F)) {
      // This memory is only accessible during hblank or vblank
      byte mode = mem_ram[LCD_STATUS_ADDR] & 0x03;
      if (mode == 0 || mode == 1) { // HBlank or VBlank
         return mem_ram[addr];
      }
      return 0xFF;
   }

   if (addr >= 0xE000 && addr <= 0xFE00) {
      addr -= 0x2000; // Mirrored memory
   }

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
