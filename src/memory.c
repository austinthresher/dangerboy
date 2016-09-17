#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"

void mem_init(void) {
   mem_bank_mode          = 0;
   mem_ram_bank_count     = 0;
   mem_rom_bank_count     = 2;
   mem_current_ram_bank   = 1;
   mem_current_rom_bank   = 0;
   mem_mbc1_extended_mode = true;
   mem_ram_bank_locked    = true;
   mem_buttons            = 0x0F;
   mem_dpad               = 0x0F;
   mem_input_last_write   = 0;

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

   mem_ram      = (byte*)calloc(0x10000, 1);
   mem_ram_bank = (byte*)calloc(0x10000, 1);
}

void mem_load_image(char* fname) {
   assert(mem_ram != NULL);
   FILE* fin = fopen(fname, "rb");
   if (fin == NULL) {
      ERROR("Could not open file %s\n", fname);
   }
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
   mem_rom = (byte*)calloc(fsize, 1024);

   fseek(fin, 0, SEEK_SET);

   i = 0;
   while (!feof(fin) && i < fsize * 1024) {
      byte   data;
      size_t bytes_read = fread(&data, 1, 1, fin);
      if (bytes_read != 1) {
         ERROR("Error reading %s\n", fname);
         exit(1);
      }
      mem_rom[i] = data;
      i++;
   }

   fclose(fin);
}

void mem_get_rom_info(void) {
   // Determine cart type. Fallthrough is intentional.
   switch (mem_rom[CART_TYPE_ADDR]) 
   {
      case 0x00: 
         // ROM Only
         mem_bank_mode = 0;
         break;
      case 0x03: 
         // ROM + MBC1 + RAM + BATT
      case 0x02:
         // ROM + MBC1 + RAM
      case 0x01:
         // ROM + MBC1
         mem_bank_mode = 1;
         break;
      case 0x06:
         // ROM + MBC2 + BATTERY
      case 0x05:
         // ROM + MBC2
         mem_bank_mode = 2;
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
         mem_bank_mode = 3;
         break;
      // MBC5 currently unsupported
      default: ERROR("Unknown banking mode: %X", mem_rom[CART_TYPE_ADDR]);
   }

   mem_rom_bank_count = (int)pow((double)2, (double)(1 + mem_rom[ROM_SIZE_ADDR]));

   // RAM Size
   switch (mem_rom[RAM_SIZE_ADDR]) 
   {
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

// Use these to implement system read writes, like OAM transfer
void mem_direct_write(word addr, byte val) {
   mem_ram[addr] = val;
}

byte mem_direct_read(word addr) {
   return mem_ram[addr];
}

// Write byte
void mem_wb(word addr, byte val) {

   if (addr < 0x8000) {
      if (mem_bank_mode == 0) {
         // Nothing under 0x8000 is writable without banking
         return;
      }
      if (addr < 0x2000) {
         if (mem_bank_mode >= 1) {
            if ((val & 0x0F) == 0x0A) {
               mem_ram_bank_locked = false;
            } else {
               mem_ram_bank_locked = true;
            }
         }
      } else if (addr < 0x4000) {
         if (mem_bank_mode == 1) {
            val &= 0x1F;
            if (val == 0) {
               val = 1;
            }
            mem_current_rom_bank = val;
         } else if (mem_bank_mode == 3) {
            val &= 0x7F;
            if (val == 0) {
               val = 1;
            }
            mem_current_rom_bank = val;
         }
      } else if (addr < 0x6000) {
         if (mem_bank_mode >= 1) {
            mem_current_ram_bank = val & 0x03;
         }
         // TODO: Bank mode 3 can also map real
         // time clock registers by writing here
      } else if (addr < 0x8000) {
         // Check if we're on MBC1+
         if (mem_bank_mode >= 1) { 
            if ((val & 0x01) == 0) {
               mem_mbc1_extended_mode = true;
            }
            if ((val & 0x01) == 1) {
               mem_mbc1_extended_mode = false;
            }
         }
      }
   } else if (addr > 0x8000 && addr < 0xA000) {
      // This memory is only accessible during hblank or vblank.
      byte mode = mem_ram[LCD_STATUS_ADDR] & 0x03;
      if (mode != 3 || (mem_ram[LCD_CONTROL_ADDR] & 0x80) == 0) { 
         mem_ram[addr] = val;
      }
   } else if (addr >= 0xA000 && addr < 0xC000) { 
      // Maybe need to check if banking is enabled here?
      if (mem_bank_mode == 0) {
         mem_ram[addr] = val;
      } else if (mem_bank_mode >= 1) {
         if (mem_ram_bank_locked == false) {
            addr -= 0xA000;
            mem_ram_bank[addr + mem_current_ram_bank * 0x2000] = val;
         }
      }
   } else if (addr >= SPRITE_RAM_START_ADDR
           && addr <= SPRITE_RAM_END_ADDR) {
      // This memory is only accessible during hblank or vblank.
      byte mode = mem_ram[LCD_STATUS_ADDR] & 0x03;
      if (mode == 0 || mode == 1 ||
          (mem_ram[LCD_CONTROL_ADDR] & 0x80) == 0) {
         mem_ram[addr] = val;
      }
   } else if (addr == DIV_REGISTER_ADDR) {
      mem_ram[DIV_REGISTER_ADDR] = 0;
   } else if (addr == LCD_SCANLINE_ADDR) {
      mem_ram[LCD_SCANLINE_ADDR] = 0;
      mem_need_reset_scanline    = true;
   } else if (addr == 0xFF46) { // OAM DMA Transfer
      word dma = val << 8;
      word ad  = SPRITE_RAM_START_ADDR;
      while (ad <= SPRITE_RAM_END_ADDR) {
         mem_ram[ad++] = mem_ram[dma++];
      }
   } else if (addr == INPUT_REGISTER_ADDR) {
      mem_input_last_write = val & 0x30;
   } else {
      if (addr >= 0xE000 && addr <= 0xFE00) {
         addr -= 0x2000; // Mirrored memory
      }
      mem_ram[addr] = val;
   }
}

byte mem_get_current_rom_bank() {
   if (mem_bank_mode < 3) {
      if (mem_mbc1_extended_mode) {
         return (mem_current_rom_bank | (mem_current_ram_bank << 5));
      }
      // This was changed to ROM bank but I think RAM is correct.
      // If anything stops working, check here. The only place this
      // is called is in mem_rb immediately below.
      return (mem_current_ram_bank & 0x1F); 
   }
   return mem_current_rom_bank;
}

// Read byte
byte mem_rb(word addr) {
   if (mem_bank_mode != 0 && addr >= 0x4000 &&
       addr < 0x8000) { // Read from the correct ROM bank
      addr -= 0x4000;
      return mem_rom[mem_get_current_rom_bank() * 0x4000 + addr];
   }
   if (mem_bank_mode != 0 && addr >= 0xA000 &&
       addr < 0xC000) { // Read from the correct RAM bank
      addr -= 0xA000;
      if (mem_mbc1_extended_mode) {
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
      // This memory is only accessible during hblank or vblank. Make sure
      // that's
      // where we are
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
