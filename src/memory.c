#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "ppu.h"

void mem_dma(byte val);

// TODO: Pokemon graphics are garbled. Fix.

void mem_init(void) {
   mem_ram              = NULL;
   mem_rom              = NULL;
   mem_ram_bank         = NULL;
   mem_rom_name         = NULL;
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

   mem_free();

   mem_ram      = (byte*)calloc(0x10000, 1);
   mem_ram_bank = (byte*)calloc(0x10000, 1);
   memset(mem_ram, 0xFF, 0x100);
   memset(mem_ram_bank, 0xFF, 0x100);
}

void mem_free() {
   if (mem_ram != NULL) {
      free(mem_ram);
      mem_ram = NULL;
   }
   if (mem_ram_bank != NULL) {
      free(mem_ram_bank);
      mem_ram_bank = NULL;
   }
   if (mem_rom != NULL) {
      free(mem_rom);
      mem_rom = NULL;
   }
   if (mem_rom_name != NULL) {
      free(mem_rom_name);
      mem_rom_name = NULL;
   }
}

void mem_load_image(char* fname) {
   assert(mem_ram != NULL);
   FILE* fin = fopen(fname, "rb");
   if (fin == NULL) {
      ERROR("Could not open file %s\n", fname);
   }

   // Load the first 32kb of the ROM to RAM
   uint32_t i = 0;
   while (!feof(fin) && i < 0x8000) {
      byte   data       = 0;
      size_t bytes_read = fread(&data, 1, 1, fin);
      if (bytes_read != 1) {
         ERROR("Error reading %s\n", fname);
      }
      mem_ram[i] = data;
      i++;
   }

   // This is in kilobytes
   int fsize = (int)pow((double)2, (double)(1 + mem_ram[0x0148])) * 16;
   mem_rom   = (byte*)calloc(fsize, 1024);

   // Store the entire cart in ROM so we can bank
   fseek(fin, 0, SEEK_SET);
   i = 0;
   while (!feof(fin) && i < fsize * 1024) {
      byte   data;
      size_t bytes_read = fread(&data, 1, 1, fin);
      if (bytes_read != 1) {
         ERROR("Error reading %s\n", fname);
      }
      mem_rom[i] = data;
      i++;
   }

   fclose(fin);
}

void mem_get_rom_info(void) {
   // Determine cart type. Fallthrough is intentional.
   switch (mem_rom[CART_TYPE_ADDR]) {
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
      default: ERROR("Unknown banking mode: %X", mem_rom[CART_TYPE_ADDR]);
   }

   mem_rom_bank_count =
      (int)pow((double)2, (double)(1 + mem_rom[ROM_SIZE_ADDR]));

   // RAM Size
   switch (mem_rom[RAM_SIZE_ADDR]) {
      case 0: mem_ram_bank_count = 0; break;
      case 1:
      case 2: mem_ram_bank_count = 1; break;
      case 3: mem_ram_bank_count = 4; break;
      case 4: mem_ram_bank_count = 16; break;
      default: ERROR("Unknown RAM bank count: %X", mem_rom[RAM_SIZE_ADDR]);
   }

   mem_current_rom_bank = 1;
   mem_current_ram_bank = 0;

   // 16 bytes at ROM_NAME_ADDR contain game title in upper case
   mem_rom_name = (char*)calloc(0x10, 1);
   memcpy(mem_rom_name, mem_rom + ROM_NAME_ADDR, 0xF);
   mem_rom_name[0xF] = '\0';
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
      mem_ram[ad++] = mem_ram[dma++];
   }
}

// Use these to implement system read writes, like OAM transfer
void mem_direct_write(word addr, byte val) {
   mem_ram[addr] = val;
}

byte mem_direct_read(word addr) {
   return mem_ram[addr];
}

// Write byte
void mem_wb(word addr, byte val) {
   cpu_advance_time(4);
   
   // Interrupt debug messages
   if (addr == INT_ENABLED_ADDR) {
      DEBUG("INT:\n"); 
      if (val & INT_VBLANK) {
         DEBUG("\t+VBLANK\n");
      } else {
         DEBUG("\t-VBLANK\n");
      }
      if (val & INT_STAT) {
         DEBUG("\t+STAT\n");
      } else {
         DEBUG("\t-STAT\n");
      }
      if (val & INT_TIMA) {
         DEBUG("\t+TIMA\n");
      } else {
         DEBUG("\t-TIMA\n");
      }
      if (val & INT_INPUT) {
         DEBUG("\t+INPUT\n");
      } else {
         DEBUG("\t-INPUT\n");
      }
   }


   if (addr < 0x8000) {
      if (mem_mbc_type == NONE) {
         cpu_advance_time(4);
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
         DEBUG("ROM Bank switched to %02X\n", mem_current_rom_bank);
      } else if (addr < 0x6000) {
         if (mem_mbc_type >= MBC1) {
            // This either selects our RAM bank for ROM4_RAM32
            // bank mode, or bits 5-6 of our ROM for ROM16_RAM8
            if (val < 0x4) {
               mem_current_ram_bank = val & 0x03;
               DEBUG("RAM Bank switched to %02X\n", mem_current_ram_bank);
            }
         }

         // TODO: MBC3 can also map real time
         // clock registers by writing here

      } else if (addr < 0x8000) {
         // This register selects our ROM / RAM banking mode
         // for MBC1 and MBC2. MBC3 uses this location to
         // latch current time into RTC registers (TODO: That.)
         if (mem_mbc_type == MBC1
          || mem_mbc_type == MBC2) {
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
      if (mode == 0 || mode == 1 || (mem_ram[LCD_CONTROL_ADDR] & 0x80) == 0) {
         mem_ram[addr] = val;
      }
   } else if (addr >= 0xA000 && addr < 0xC000) {
      // Maybe need to check if banking is enabled here?
      if (mem_mbc_type == NONE) {
         mem_ram[addr] = val;
      } else if (mem_mbc_type >= MBC1) {
         if (mem_ram_bank_locked == false) {
            addr -= 0xA000;
            mem_ram_bank[addr + mem_current_ram_bank * 0x2000] = val;
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
            DEBUG("DIV RESET\n");
            mem_ram[DIV_REGISTER_ADDR] = 0;
            break;
         case TIMA_ADDR:
            DEBUG("TIMA WRITE: %d\n", val);
            mem_ram[addr] = val;
            break;
         case TIMER_CONTROL_ADDR:
            if (val & 0x04) {
               DEBUG("TIMER ENABLED\n");
            } else {
               DEBUG("TIMER DISABLED\n");
            }
            if (val & 0x3) {
               DEBUG("TIMER MODE: %d\n", val & 3);
               switch (val & 3) {
                  case 0: cpu_tima_timer = 1024; break;
                  case 1: cpu_tima_timer = 16;   break;
                  case 2: cpu_tima_timer = 64;   break;
                  case 3: cpu_tima_timer = 256;  break;
                  default: ERROR("timer error");
               }
            }
            mem_ram[addr] = val;
            break;
         case LCD_CONTROL_ADDR:
         case LCD_STATUS_ADDR:
         case LCD_SCY_ADDR:
         case LCD_SCX_ADDR:
         case LCD_LINE_Y_ADDR:
         case LCD_LINE_Y_C_ADDR:
            ppu_update_register(addr, val);
            break;
         case OAM_DMA_ADDR:
            mem_dma(val);
           break;
         case INPUT_REGISTER_ADDR:
            mem_input_last_write = val & 0x30;
            break;
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
         byte bank = mem_current_rom_bank | (mem_current_ram_bank << 5);
         if (bank == 0) {
            bank = 1;
         }
         if (bank == 0x20) {
            bank = 0x21;
         }
         if (bank == 0x40) {
            bank = 0x41;
         }
         if (bank == 0x60) {
            bank = 0x61;
         }
         return bank;
      } else {
         byte bank = mem_current_rom_bank & 0x1F;
         if (bank == 0) {
            bank = 1;
         }
         return bank;
      }
   }
   // TODO: Make sure MBC2 doesn't need special behavior here
   // Other MBCs
   if (mem_current_rom_bank & 0x7F == 0) {
      return 1;
   }
   return mem_current_rom_bank & 0x7F;
}

// Read byte
byte mem_rb(word addr) {
   cpu_advance_time(4);

   // 0x0000 to 0x3FFF always contains the first 16 kb of ROM
   if (addr < 0x4000) {
      return mem_ram[addr];
   }

   // 0x4000 to 0x7FFF can contain any other ROM bank
   if (mem_mbc_type != NONE && addr >= 0x4000 && addr < 0x8000) {
      addr -= 0x4000;
      return mem_rom[mem_get_current_rom_bank() * 0x4000 + addr];
   }

   // 0xA000 to 0xBFFF is used for external RAM, usually SRAM
   // TODO: Make sure this works with MBC2, which has 512x4 bits
   // of "external" RAM. The full address space is not used there.
   if (mem_mbc_type != NONE && addr >= 0xA000 && addr < 0xC000) {

      // The external RAM could be up to 2kb, 8kb, or 32kb
      // 32kb mode required 4 RAM banks
      addr -= 0xA000;
      if (mem_mbc_type < MBC3 && mem_mbc_bankmode == ROM4_RAM32) {
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
      byte mode = mem_ram[0xFF41] & 0x03;
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
