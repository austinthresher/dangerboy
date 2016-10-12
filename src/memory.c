#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "apu.h"
#include "debugger.h"
#include "lcd.h"
#include "memory.h"

typedef enum mbc_type_ { NONE = 0, MBC1 = 1, MBC2 = 2, MBC3 = 3 } mbc_type;

typedef enum mbc_bankmode_ { ROM16_RAM8 = 0, ROM4_RAM32 = 1 } mbc_bankmode;

typedef enum dma_state_ {
   INACTIVE,
   STARTING,
   ACTIVE,
   RESTARTING,
} dma_state;

// ------------------
// Internal variables
// ------------------

dma_state dma;
mbc_type mbc;
mbc_bankmode banking;
word dma_src, dma_dst, dma_rst;
bool ram_locked;
byte rom_banks;
byte ram_banks;
byte rom_bank;
byte ram_bank;
byte* ram;
byte* rom;
byte* banked_ram;
char rom_name[16];
byte joy_dpad;
byte joy_buttons;
byte joy_last_write;

// ------------------
// Internal functions
// ------------------

void start_dma(byte val);
byte get_rom_bank();

// --------------------
// Function definitions
// --------------------

void dwrite(word addr, byte val) {
   ram[addr] = val;
}

byte dread(word addr) {
   return ram[addr];
}

void press_button(button but) {
   joy_buttons &= ~but;
   wbyte(IF, rbyte(IF) | INT_INPUT);
}

void press_dpad(dpad dir) {
   joy_dpad &= ~dir;
   wbyte(IF, rbyte(IF) | INT_INPUT);
}

void release_button(button but) {
   joy_buttons |= but;
}

void release_dpad(dpad dir) {
   joy_dpad |= dir;
}

void mem_init(void) {
   mem_free();
   dma_dst        = 0;
   dma_src        = 0;
   dma_rst        = 0;
   mbc            = NONE;
   ram_banks      = 0;
   rom_banks      = 2;
   ram_bank       = 1;
   rom_bank       = 0;
   banking        = ROM16_RAM8;
   ram_locked     = true;
   joy_buttons    = 0x0F;
   joy_dpad       = 0x0F;
   joy_last_write = 0;
   dma            = INACTIVE;
   ram            = (byte*)calloc(0x10000, 1);
   banked_ram     = (byte*)calloc(0x10000, 1);
}

void mem_free() {
   if (ram != NULL) {
      free(ram);
   }
   ram = NULL;

   if (banked_ram != NULL) {
      free(banked_ram);
   }
   banked_ram = NULL;

   if (rom != NULL) {
      free(rom);
   }
   rom = NULL;
}

void mem_load_image(char* fname) {
   assert(ram != NULL);
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
      ram[i] = data;
      i++;
   }

   int rom_size = dread(ROMSIZE);
   if (rom_size < 8) {
      rom_banks = pow(2, rom_size + 1);
      rom_size  = rom_banks * 0x4000;
      rom       = calloc(rom_size, 1);
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
      rom[i] = data;
      i++;
   }

   fclose(fin);

   // Determine cart type. Fallthrough is intentional.
   switch (ram[CARTTYPE]) {
      case 0x00:
         // ROM Only
         mbc = NONE;
         break;
      case 0x03:
      // ROM + MBC1 + RAM + BATT
      case 0x02:
      // ROM + MBC1 + RAM
      case 0x01:
         // ROM + MBC1
         mbc = MBC1;
         break;
      case 0x06:
      // ROM + MBC2 + BATTERY
      case 0x05:
         // ROM + MBC2
         mbc = MBC2;
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
         mbc = MBC3;
         break;
      // MBC5 currently unsupported
      default:
         fprintf(stderr,
               "Unknown banking mode: %X. Attempting to use MBC1.",
               ram[CARTTYPE]);
         mbc = MBC1;
         break;
   }

   // RAM Size
   switch (ram[RAMSIZE]) {
      case 0:
         ram_banks = 0;
         break;
      case 1:
      case 2:
         ram_banks = 1;
         break;
      case 3:
         ram_banks = 4;
         break;
      case 4:
         ram_banks = 16;
         break;
      default:
         fprintf(stderr,
               "Unknown RAM bank count: %X. Attempting to use 16.",
               ram[RAMSIZE]);
         ram_banks = 16;
         break;
   }

   rom_bank = 1;
   ram_bank = 0;

   // 16 bytes at ROMNAME contain game title in upper case
   memcpy(rom_name, ram + ROMNAME, 14);
   rom_name[15] = '\0';
}

void mem_print_rom_info() {
   printf("Name:\t\t%s\n", rom_name);
   printf("MBC:\t\t%d\n", mbc);
   printf("ROM Banks:\t%d\n", rom_banks);
   printf("RAM Banks:\t%d\n", ram_banks);
}

void start_dma(byte val) {
   if (dma == INACTIVE) {
      dbg_log("OAM DMA Starting");
      dma     = STARTING;
      dma_src = val << 8;
      dma_dst = OAMSTART;
      ;
   } else {
      dbg_log("OAM DMA Restarting");
      dma     = RESTARTING;
      dma_rst = val << 8;
   }
}

void mem_advance_time(cycle ticks) {
   if (dma != INACTIVE) {
      while (ticks > 0) {
         ticks -= 4;
         if (dma == STARTING) {
            dbg_log("OAM DMA Started");
            dma = ACTIVE;
            continue;
         }

         dwrite(dma_dst, rbyte(dma_src));

         dma_dst++;
         dma_src++;
         if (dma_dst >= OAMEND + 1 && dma != RESTARTING) {
            dma_dst = 0;
            dma_src = 0;
            dbg_log("OAM DMA Finish");
            dma = INACTIVE;
            break;
         }

         if (dma == RESTARTING) {
            dbg_log("OAM DMA Restarted");
            dma     = ACTIVE;
            dma_src = dma_rst;
            dma_dst = OAMSTART;
            continue;
         }
      }
   }
}

byte get_rom_bank() {
   if (mbc == MBC1) {
      // In ROM16_RAM8 mode, ram_bank
      // holds bits 5-6 of our ROM bank index.
      if (banking == ROM16_RAM8) {
         byte bank = (rom_bank & 0x1F) | ((ram_bank & 0x3) << 5);
         return bank;
      } else {
         byte bank = rom_bank & 0x1F;
         return bank;
      }
   }
   // TODO: Make sure MBC2 doesn't need special behavior here
   // Other MBCs
   if ((rom_bank & 0x7F) == 0) {
      return 1;
   }
   return rom_bank & 0x7F;
}

// Write byte
void wbyte(word addr, byte val) {
   dbg_notify_write(addr, val);

   switch (addr & 0xF000) {
      case 0x0000: // External ram enable / disable
      case 0x1000:
         if (mbc != NONE) {
            // MBC2 has the restriction that the least significant
            // bit of the upper address byte must be zero
            if (mbc != MBC2 || (addr & 0x100) == 0) {

               // Writing to this region of memory enables or
               // disables external RAM, based on the value
               if ((val & 0x0F) == 0x0A) {
                  ram_locked = false;
               } else {
                  ram_locked = true;
               }
            }
         }
         return;
      case 0x2000: // ROM bank select
      case 0x3000:
         if (mbc == NONE) {
            return;
         }
         if (mbc == MBC1) {
            // Writing to this region selects the lower 5 bits
            // of the ROM bank index
            val &= 0x1F;
            if (val == 0) {
               val = 1;
            }
            rom_bank = val;
            return;
         }
         if (mbc == MBC2) {
            // For MBC2, the least significant bit of the upper
            // address byte must be one
            if (addr & 0x0100) {
               val &= 0x0F;
               if (val == 0) {
                  val = 1;
               }
               rom_bank = val;
            }
            return;
         }
         if (mbc == MBC3) {
            // MBC3 uses all 7 bits here instead of splitting
            // into hi / lo writes
            val &= 0x7F;
            rom_bank = val;
            return;
         }
         return;
      case 0x4000: // RAM bank select
      case 0x5000:
         if (mbc != NONE) {
            // This either selects our RAM bank for ROM4_RAM32
            // bank mode, or bits 5-6 of our ROM for ROM16_RAM8
            if (mbc != MBC3 || val < 0x4) {
               ram_bank = val & 0x03;
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
         if (mbc == MBC1 || mbc == MBC2) {
            if ((val & 0x01) == 0) {
               banking = ROM16_RAM8;
            }
            if ((val & 0x01) == 1) {
               banking = ROM4_RAM32;
            }
         }
         return;
      case 0x8000: // VRAM
      case 0x9000:
         if (lcd_vram_accessible()) {
            ram[addr] = val;
         }
         return;
      case 0xA000: // External RAM
      case 0xB000:
         if (mbc == NONE) {
            ram[addr] = val;
            return;
         }
         if (ram_locked == false) {
            addr -= 0xA000;
            if (mbc == MBC3 || banking == ROM4_RAM32) {
               banked_ram[(addr + ram_bank * 0x2000) & 0xFFFF] = val;
               return;
            }
            banked_ram[addr] = val;
         }
         return;
      case 0xC000: // Work RAM
      case 0xD000:
         ram[addr] = val;
         return;
      case 0xE000: // 0xC000 mirror
         ram[addr - 0x2000] = val;
         return;
      case 0xF000:
         if (addr < 0xFE00) { // Partial 0xD000 mirror
            ram[addr - 0x2000] = val;
            return;
         }
         if (addr < 0xFEA0) { // FE00 to FE9F is OAM
            // OAM can only be written to during HBLANK or VBLANK,
            // and not during an OAM DMA transfer
            if (lcd_oam_accessible()) {
               if (dma == INACTIVE || dma == STARTING) {
                  ram[addr] = val;
               }
            }
            return;
         }
         break;
      default:
         break;
   }
   // We will only reach here if our address wasn't handled
   // in the 0xF000 case. We can assume we're dealing with
   // HRAM or hardware registers.
   if (addr >= 0xFEA0 && addr < 0xFEFF) {
      return; // This memory is not usable
   }

   // HW Registers
   switch (addr) {
      case DIV:
         // TIMA and DIV use the same internal counter,
         // so resetting DIV also resets TIMA
         cpu_reset_timer();
         ram[DIV] = 0;
         return;
      case DMA:
         start_dma(val);
         return;
      case JOYP:
         joy_last_write = val & 0x30;
         return;

      // LCD HW Registers
      case LCDC:
      case STAT:
      case SCY:
      case SCX:
      case LY:
      case LYC:
         lcd_reg_write(addr, val);
         return;

      // Audio HW Registers
      case CH1SWEEP:
      case CH1LENGTH:
      case CH1VOLUME:
      case CH1LOFREQ:
      case CH1HIFREQ:
      case CH2LENGTH:
      case CH2VOLUME:
      case CH2LOFREQ:
      case CH2HIFREQ:
      case CH3ENABLE:
      case CH3LENGTH:
      case CH3VOLUME:
      case CH3LODATA:
      case CH3HIDATA:
      case CH4LENGTH:
      case CH4VOLUME:
      case CH4POLY:
      case CH4CONSEC:
      case WAVETABLE:
         apu_reg_write(addr, val);
         return;

      default:
         break;
   }

   // 0xFF00 to 0xFFFF
   ram[addr] = val;
}

// Read byte
byte rbyte(word addr) {
   dbg_notify_read(addr);

   switch (addr & 0xF000) {
      case 0x0000: // ROM bank 0
      case 0x1000:
      case 0x2000:
      case 0x3000:
         return rom[addr];
      case 0x4000: // Rom banks 1-X
      case 0x5000:
      case 0x6000:
      case 0x7000:
         if (mbc == NONE) {
            return rom[addr];
         }
         addr -= 0x4000;
         return rom[(get_rom_bank() % rom_banks) * 0x4000 + addr];
      case 0x8000: // VRAM
      case 0x9000:
         if (lcd_vram_accessible()) {
            return ram[addr];
         }
         return 0xFF;
      case 0xA000: // External RAM
      case 0xB000:
         if (mbc == NONE) {
            return ram[addr];
         }
         if (mbc == MBC3 || banking == ROM4_RAM32) {
            return banked_ram[(addr - 0xA000 + ram_bank * 2000) & 0xFFFF];
         }
         return banked_ram[addr - 0xA000];
      case 0xC000: // Work RAM
      case 0xD000:
         return ram[addr];
      case 0xE000: // Mirror of 0xC000
         return ram[addr - 0x2000];
      case 0xF000:
         if (addr < 0xFE00) { // Partial 0xD000 mirror
            return ram[addr - 0x2000];
         }
         if (addr < 0xFEA0) { // FE00 to FE9F is OAM
            // OAM can only be read during HBLANK or VBLANK,
            // and not during an OAM DMA transfer
            if (lcd_oam_accessible()) {
               if (dma == INACTIVE || dma == STARTING) {
                  return ram[addr];
               }
            }
            return 0xFF;
         }
         break;
      default:
         break;
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
         if (joy_last_write == 0x00) {
            return 0xC0 | (joy_dpad & joy_buttons);
         }
         if (joy_last_write == 0x10) {
            return 0xC0 | joy_buttons;
         }
         if (joy_last_write == 0x20) {
            return 0xC0 | joy_dpad;
         }
         return 0xFF;
      case IE:
         return 0xE0 | (ram[IE] & 0x1F);
      case IF:
         return 0xE0 | (ram[IF] & 0x1F);

      // LCD HW Registers
      case LCDC:
      case STAT:
      case SCY:
      case SCX:
      case LY:
      case LYC:
         return lcd_reg_read(addr);

      // Audio HW Registers
      case CH1SWEEP:
      case CH1LENGTH:
      case CH1VOLUME:
      case CH1LOFREQ:
      case CH1HIFREQ:
      case CH2LENGTH:
      case CH2VOLUME:
      case CH2LOFREQ:
      case CH2HIFREQ:
      case CH3ENABLE:
      case CH3LENGTH:
      case CH3VOLUME:
      case CH3LODATA:
      case CH3HIDATA:
      case CH4LENGTH:
      case CH4VOLUME:
      case CH4POLY:
      case CH4CONSEC:
      case WAVETABLE:
         return apu_reg_read(addr);

      default:
         break;
   }

   // HRAM and other registers
   return ram[addr];
}

// Write word
void wword(word addr, word val) {
   wbyte(addr, val & 0xFF);
   wbyte(addr + 1, val >> 8);
}

// Read word
word rword(word addr) {
   return rbyte(addr) | (rbyte(addr + 1) << 8);
}
