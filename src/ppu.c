#include "ppu.h"
#include "debugger.h"

byte mode;
byte win_y;
tick timer;
int scroll_tick_mod;

void update_scroll_mod();
void set_ly(int ly);
void check_lyc();

void fire_stat() {
   mem_wb(INT_FLAG_ADDR, mem_direct_read(INT_FLAG_ADDR) | INT_STAT);
}

void ppu_init() {
   mode        = PPU_MODE_SCAN_OAM;
   timer       = 0;
   ppu_ly      = 0;
   lcd_disable = false;
   ppu_draw    = false;
   ppu_win_ly  = 0;
   ppu_reset();
}

tick ppu_get_timer() { return timer; }

void ppu_reset() {
   mode   = PPU_MODE_HBLANK;
   ppu_ly = 0;
   ppu_win_ly = 0;
   set_ly(0);
}

// Checks for LY == LYC, updates STAT bit, fires interrupt if needed 
void check_lyc() {
    // If lyc == scanline and bit 6 of stat is set, interrupt
   byte ly = mem_direct_read(LCD_LINE_Y_ADDR);
   byte lyc = mem_direct_read(LCD_LINE_Y_C_ADDR);
   byte stat = mem_direct_read(LCD_STATUS_ADDR);

   if (ly == lyc) {
      stat |= 0x04; // LYC bit
      if ((stat & 0x02) == 0) { // Only if in HBLANK or VBLANK
         if (stat & 0x40) {
            fire_stat();
         }
      }
   } else {
      stat &= ~0x04; // Clear LYC bit
   }
   mem_direct_write(LCD_STATUS_ADDR, stat);
}

void set_ly(int ly) {
   mem_direct_write(LCD_LINE_Y_ADDR, ly);
   debugger_notify_mem_write(LCD_LINE_Y_ADDR, ly);
   check_lyc();
}

void update_stat_mode() {
   // This replaces the bottom 2 bits in the STAT register
   // with the current value of mode
   mem_direct_write(LCD_STATUS_ADDR,
      (mem_direct_read(LCD_STATUS_ADDR) & ~3) | (mode & 3));
}

void ppu_update_register(word addr, byte val) {
   switch (addr) {
      case LCD_LINE_Y_ADDR:
         ppu_ly     = 0;
         ppu_win_ly = 0;
         // This will check LY == LYC, make sure that writing
         // to LY is supposed to fire LY == LYC interrupt
         set_ly(0); 
         break;
      case LCD_LINE_Y_C_ADDR:
         mem_direct_write(LCD_LINE_Y_C_ADDR, val);
         check_lyc();
         break;
      case LCD_CONTROL_ADDR:
         mem_direct_write(LCD_CONTROL_ADDR, val);
         if (val & 0x80) {
            if (lcd_disable) {
               mode = PPU_MODE_SCAN_OAM;
               timer = 0; // Confirm that timer should reset here
            }
            lcd_disable = false;
            update_stat_mode();
         } else {
            if (!lcd_disable) {
               ppu_draw = true;
            }
            lcd_disable = true;
            mode &= 2; // Bit 2 is preserved when disabling LCD
            update_stat_mode();
            set_ly(0);
         }
         break;
      case LCD_STATUS_ADDR:
         // Clear the mode bits and the unused 7th bit
         val &= ~0x87; 

         // Write to the interrupt enable flags, but not to mode or lyc
         mem_direct_write(LCD_STATUS_ADDR,
               mem_direct_read(LCD_STATUS_ADDR) & 0x7 | val);
         
         // Writing to stat during vblank is supposed to raise an interrupt
         if (mode == PPU_MODE_VBLANK) {
            fire_stat();
         }
         break;
      default: mem_direct_write(addr, val);
   }
}

// Based on Mooneye's implementation
void update_scroll_mod() {
   byte scx = mem_direct_read(LCD_SCX_ADDR) % 8;
   if (scx > 4) {
      scroll_tick_mod = 8;
   } else if (scx > 0) {
      scroll_tick_mod = 4;
   } else {
      scroll_tick_mod = 0;
   }
}

void ppu_advance_time(tick ticks) {
   if (lcd_disable) {
      check_lyc();
      return;
   }

   int vram_length = 172 + scroll_tick_mod;
   tick old_timer  = timer;
   byte stat_reg   = mem_direct_read(LCD_STATUS_ADDR);

   timer += ticks;

   switch (mode) {
      case PPU_MODE_SCAN_OAM:
         if (timer >= 84) {
            mode = PPU_MODE_SCAN_VRAM;
            timer -= 84;
            update_scroll_mod();
            update_stat_mode();
         }
         break;

      case PPU_MODE_SCAN_VRAM:
         // According to Mooneye tests, HBLANK STAT interrupt is 4 cycles early
         if (old_timer < vram_length - 4 && timer >= vram_length - 4) {
            // Check if HBLANK STAT interrupt is enabled
            if (stat_reg & 0x08) {
               // LY == LYC interrupt masks HBLANK interrupt
               if ((stat_reg & 0x44) != 0x44) {
                  fire_stat();
               }
            }
         }
         if (timer >= vram_length) {
            mode = PPU_MODE_HBLANK;
            timer -= vram_length;
            ppu_do_scanline();
            update_stat_mode();
         }
         break;

      case PPU_MODE_HBLANK:
         if (timer >= 200 - scroll_tick_mod) {
            timer -= 200 - scroll_tick_mod;
            ppu_ly++;
            set_ly(ppu_ly);
            if (ppu_ly == 144) {
               mode     = PPU_MODE_VBLANK;
               ppu_draw = true;
               mem_wb(INT_FLAG_ADDR,
                     mem_direct_read(INT_FLAG_ADDR) | INT_VBLANK);
               update_stat_mode();
               // Fire VBLANK STAT interrupt if enabled
               if (stat_reg & 0x10) {
                  fire_stat();
               } else if (stat_reg & 0x20) {
                  // OAM interrupt still fires on ly == 144,
                  // but only if STAT VBLANK interrupt did not
                  // TODO: Should normal VBLANK mask this as well?
                  fire_stat();
               }
            } else {
               mode = PPU_MODE_SCAN_OAM;
               update_stat_mode();
               // Check if OAM STAT interrupt is enabled
               if (stat_reg & 0x20) {
                  // HBLANK STAT interrupt masks OAM interrupt
                  if (!(stat_reg & 0x08)) {
                     // So does LYC == LY interrupt
                     if((stat_reg & 0x44) != 0x44) {
                        fire_stat();
                     }
                  }
               }
            }
         }
         break;
      case PPU_MODE_VBLANK:
         if (timer >= 456) {
            timer -= 456;
            ppu_ly++;
            if (ppu_ly > 153) {
               ppu_ly = 0;
               ppu_win_ly = 0;
               mode = PPU_MODE_SCAN_OAM;
               update_stat_mode();
               // Check if OAM STAT interrupt is enabled
               if (stat_reg & 0x20) {
                  // HBLANK STAT interrupt masks OAM interrupt
                  if (!(stat_reg & 0x08)) {
                     // So does LYC == LY interrupt
                     if((stat_reg & 0x44) != 0x44) {
                        fire_stat();
                     }
                  }
               }
            }
            set_ly(ppu_ly);
         }
         break;
   }
}


void ppu_do_scanline() {
   bool bg_is_zero[160];

   bool window   = false;
   int vram_addr = ppu_ly * 160 * 3;

   byte win_x = mem_direct_read(WIN_X_ADDR);
   if (ppu_ly == 0) {
      win_y = mem_direct_read(WIN_Y_ADDR);
   }
   bool bg_enabled   = mem_direct_read(LCD_CONTROL_ADDR) & 0x01;
   bool win_tile_map = mem_direct_read(LCD_CONTROL_ADDR) & 0x40;
   bool bg_tile_map  = mem_direct_read(LCD_CONTROL_ADDR) & 0x08;
   bool tile_bank    = mem_direct_read(LCD_CONTROL_ADDR) & 0x10;
   byte scroll_x     = mem_direct_read(LCD_SCX_ADDR);
   byte scroll_y     = mem_direct_read(LCD_SCY_ADDR);
   word bg_map_loc   = 0x9800 + (bg_tile_map ? 0x400 : 0);
   word win_map_loc  = 0x9800 + (win_tile_map ? 0x400 : 0);
   word bg_map_off   = (((ppu_ly + scroll_y) & 0xFF) >> 3) * 32;
   word win_map_off  = ((ppu_win_ly & 0xFF) >> 3) * 32;
   byte bg_x_off     = (scroll_x >> 3) & 0x1F;
   byte win_x_off    = 0;
   byte win_x_px     = 0;
   byte bg_tile      = mem_direct_read(bg_map_loc + bg_map_off + bg_x_off);
   byte win_tile     = mem_direct_read(win_map_loc + win_map_off + win_x_off);

   byte bg_xpx_off  = scroll_x & 0x07;
   byte bg_ypx_off  = (scroll_y + ppu_ly) & 0x07;
   byte win_xpx_off = 0;
   byte win_ypx_off = ppu_win_ly & 0x07;
   byte start_x_off = bg_xpx_off;
   byte outcol      = 0;

   for (int i = 0; i < 160; i++) {
      if ((mem_direct_read(LCD_CONTROL_ADDR) & 0x20)
            && ppu_ly >= win_y
            && i >= win_x - 7
            && win_x < 166) {
         window = true;
      }

      byte tile_index = window ? win_tile : bg_tile;
      int tile_addr;
      if (tile_index < 128) {
         if (tile_bank) {
            tile_addr = 0x8000 + tile_index * 16;
         } else {
            tile_addr = 0x9000 + tile_index * 16;
         }
      } else {
         tile_addr = 0x8800 + (tile_index - 128) * 16;
      }

      if (!window) {
         tile_addr += bg_ypx_off * 2;
      } else {
         tile_addr += win_ypx_off * 2;
      }

      byte mask = 1;
      if (!window) {
         mask <<= 7 - ((i + start_x_off) & 0x07);
      } else {
         mask <<= 7 - ((win_x_px++) & 0x07);
      }

      byte hi  = mem_direct_read(tile_addr + 1);
      byte lo  = mem_direct_read(tile_addr);
      byte col = 0;
      if (hi & mask) {
         col = 2;
      }
      if (lo & mask) {
         col += 1;
      }
      
      if (bg_enabled || window) {
         bg_is_zero[i]         = col == 0;
         outcol                = ppu_pick_color(col, mem_direct_read(BG_PAL_ADDR));
         if (vram_addr + 2 < 160 * 144 * 3) {
            ppu_vram[vram_addr++] = outcol;
            ppu_vram[vram_addr++] = outcol;
            ppu_vram[vram_addr++] = outcol;
         }

         if (!window) {
            bg_xpx_off++;
            if (bg_xpx_off >= 8) {
               bg_xpx_off = 0;
               bg_x_off++;
               bg_x_off &= 0x1F;
               bg_tile = mem_direct_read(bg_map_loc + bg_map_off + bg_x_off);
            }
         } else {
            win_xpx_off++;
            if (win_xpx_off >= 8) {
               win_xpx_off = 0;
               win_x_off++;
               win_x_off &= 0x1F;
               win_tile = mem_direct_read(win_map_loc + win_map_off + win_x_off);
            }
         }
      } else {
         bg_is_zero[i] = true;
      }
   }

   if (window) {
      ppu_win_ly++;
   }

   // Check if sprites are enabled
   if (!(mem_direct_read(LCD_CONTROL_ADDR) & 0x02)) {
      return;
   }

   // Begin sprite drawing
   bool big_sprites = mem_direct_read(LCD_CONTROL_ADDR) & 0x04;
   for (int spr = 0; spr < 40; spr++) {
      byte y     = mem_direct_read(SPRITE_RAM_START_ADDR + spr * 4);
      byte x     = mem_direct_read(SPRITE_RAM_START_ADDR + spr * 4 + 1);
      byte tile  = mem_direct_read(SPRITE_RAM_START_ADDR + spr * 4 + 2);
      byte attr  = mem_direct_read(SPRITE_RAM_START_ADDR + spr * 4 + 3);
      bool pal   = attr & 0x10;
      bool xflip = attr & 0x20;
      bool yflip = attr & 0x40;
      bool pri   = attr & 0x80;
      int draw_x = x - 8;
      int draw_y = y - 16;
      int height = big_sprites ? 16 : 8;

      if (x == 0 || y == 0 || y >= 160 || x >= 168) {
         continue;
      }

      if (draw_y <= ppu_ly && draw_y + height > ppu_ly) {
         byte spr_index = tile;
         // In 8x16 mode, the least significant bit is ignored
         if (big_sprites) {
            spr_index &= 0xFE;
         }

         byte spr_line = ppu_ly - draw_y;
         if (yflip) {
            spr_line = height - 1 - spr_line;
         }

         byte y_mask   = height - 1;
         word spr_addr = 0x8000 + spr_index * 16 + (spr_line & y_mask) * 2;
         byte shi      = mem_direct_read(spr_addr + 1);
         byte slo      = mem_direct_read(spr_addr);

         for (int sx = 0; sx < 8; sx++) {
            byte smask;
            if (!xflip) {
               smask = 0x80 >> sx;
            } else {
               smask = 0x01 << sx;
            }
            byte scol = 0;
            if (shi & smask) {
               scol = 2;
            }
            if (slo & smask) {
               scol++;
            }
            outcol = ppu_pick_color(scol, mem_direct_read(OBJ_PAL_ADDR + pal));

            if (draw_x + sx < 160 && draw_x + sx >= 0 && scol) {
               if (pri == 1 && !bg_is_zero[draw_x + sx]) {
                  continue;
               }
               int output_addr = (ppu_ly * 160 + draw_x + sx) * 3;
               if (output_addr + 2 < 160 * 144 * 3) {
                  ppu_vram[output_addr++] = outcol;
                  ppu_vram[output_addr++] = outcol;
                  ppu_vram[output_addr++] = 0;//outcol;
               }
            }
         }
      }
   }
}

// TODO:
byte ppu_pick_color(byte col, byte pal) {
   byte temp = (pal >> (col * 2)) & 0x03;
   switch (temp) {
      case 3: return LCD_BLACK;
      case 2: return LCD_DARK;
      case 1: return LCD_LITE;
      case 0: return LCD_WHITE;
   }
   return LCD_WHITE;
}
