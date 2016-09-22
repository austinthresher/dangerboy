#include "ppu.h"

byte mode;
byte win_y;
bool lcd_disable;
tick timer;

void ppu_init() {
   mode              = PPU_MODE_SCAN_OAM;
   timer             = 0;
   ppu_ly            = 0;
   lcd_disable       = false;
   ppu_ready_to_draw = false;
   ppu_vram          = NULL;
   ppu_win_ly        = 0;
   ppu_reset();
}

void ppu_reset() {
   if (ppu_vram != NULL) {
      free(ppu_vram);
   }

   ppu_vram = (byte*)calloc(160 * 144, 3);
   for (int i = 0, n = 0; i < 40; i++, n += 4) {
      mem_direct_write(SPRITE_RAM_START_ADDR + n + 0, 0);
      mem_direct_write(SPRITE_RAM_START_ADDR + n + 1, 0);
      mem_direct_write(SPRITE_RAM_START_ADDR + n + 2, 0);
      mem_direct_write(SPRITE_RAM_START_ADDR + n + 3, 0);
   }
}

void ppu_update_stat() {
   
   // If the LCD is disabled, freeze our status
   // just before we jump back to OAM mode. We
   // Stay in VBLANK until the LCD is enabled.
   if ((mem_rb(LCD_CONTROL_ADDR) & 0x80) == 0) {
      if (!lcd_disable) {
         lcd_disable = true;
         ppu_ready_to_draw = true;
      }
      timer = 456;
      mode = PPU_MODE_VBLANK;
      ppu_ly = 153;
      mem_direct_write(LCD_LINE_Y_ADDR, 0);
      byte stat = mem_direct_read(LCD_STATUS_ADDR) & 0xFC;
      stat |= 1;
      mem_direct_write(LCD_STATUS_ADDR, stat);
      return;
   }

   lcd_disable = false;

   // Writing here normally resets it, so we set it directly
   mem_direct_write(LCD_LINE_Y_ADDR, ppu_ly);

   // If lyc == scanline and bit 6 of stat is set, interrupt
   byte lyc = mem_direct_read(LCD_LINE_Y_C_ADDR);

   // Bits 0-2 indicate the gpu mode.
   // Higher bits are interrupt enable flags.
   byte new_stat = (ppu_ly == lyc ? 0x04 : 0) | (mode & 0x03);
   byte stat_reg = mem_direct_read(LCD_STATUS_ADDR);
   mem_direct_write(LCD_STATUS_ADDR, (stat_reg & 0xF8) | (new_stat & 0x07));

   // The STAT interrupt has 4 different modes, based on bits 3-6
   if ((mode == PPU_MODE_HBLANK   && (stat_reg & 0x08))
    || (mode == PPU_MODE_VBLANK   && (stat_reg & 0x10))
    || (mode == PPU_MODE_SCAN_OAM && (stat_reg & 0x20))
    || (ppu_ly == lyc             && (stat_reg & 0x40))) {
      mem_direct_write(INT_FLAG_ADDR, mem_direct_read(INT_FLAG_ADDR) | INT_STAT);
   } else if (ppu_ly == 144) {
      // The OAM interrupt can still fire on scanline 144. This only
      // happens if the STAT VBlank interrupt did not fire.
      // TODO: Does this happen if normal VBlank interrupt fires?
      if (stat_reg & 0x20 && stat_reg & 0x10 == 0) {
         mem_direct_write(INT_FLAG_ADDR, mem_direct_read(INT_FLAG_ADDR) | INT_STAT);
      }
   }
}

void ppu_advance_time(tick ticks) {
   timer += ticks;

   switch (mode) {
      case PPU_MODE_SCAN_OAM:
         if (timer >= 80) {
            mode = PPU_MODE_SCAN_VRAM;
            timer = 0;
            ppu_update_stat();
         }
         break;
      case PPU_MODE_SCAN_VRAM:
         if (timer >= 172) {
            mode = PPU_MODE_HBLANK;
            timer = 0;
            ppu_do_scanline();
            ppu_update_stat(); 
         }
         break;
      case PPU_MODE_HBLANK:
         if (timer >= 204) {
            timer = 0;
            ppu_ly++;
            if (ppu_ly > 143) {
               mode = PPU_MODE_VBLANK;
               ppu_ready_to_draw = true;
               mem_direct_write(INT_FLAG_ADDR,
                     mem_direct_read(INT_FLAG_ADDR) | INT_VBLANK);
            }
            else {
               mode = PPU_MODE_SCAN_OAM;
            }
            ppu_update_stat();
         }
         break;
      case PPU_MODE_VBLANK:
         if (timer >= 456) {
            timer = 0;
            ppu_ly++;
            if (ppu_ly > 153) {
               mode = PPU_MODE_SCAN_OAM;
               ppu_ly = 0;
               ppu_win_ly = 0;
            }
            ppu_update_stat();
         }
         break;
   }
}

void ppu_do_scanline() {
   bool window = false;
   int  vram_addr = ppu_ly * 160 * 3;

   if (lcd_disable) {
      for (int i = 0; i < 160; i++) {
         ppu_vram[vram_addr++] = 0xFF;
         ppu_vram[vram_addr++] = 0xFF;
         ppu_vram[vram_addr++] = 0xFF;
      }
      return;
   }

   byte win_x = mem_direct_read(WIN_X_ADDR);
   if (ppu_ly == 0) {
      win_y = mem_direct_read(WIN_Y_ADDR);
   }
      
   bool bg_is_zero[160];
   byte scroll_x     = mem_direct_read(LCD_SCX_ADDR);
   byte scroll_y     = mem_direct_read(LCD_SCY_ADDR);
   bool win_tile_map = mem_direct_read(LCD_CONTROL_ADDR) & 0x40;
   bool bg_tile_map  = mem_direct_read(LCD_CONTROL_ADDR) & 0x08;
   bool tile_bank    = mem_direct_read(LCD_CONTROL_ADDR) & 0x10;
   word bg_map_loc   = 0x9800 + bg_tile_map  * 0x400;
   word win_map_loc  = 0x9800 + win_tile_map * 0x400;
   word bg_map_off   = (((ppu_ly + scroll_y) & 0xFF) >> 3) * 32;
   word win_map_off  = ((ppu_win_ly & 0xFF) >> 3) * 32;
   byte bg_x_off     = (scroll_x >> 3) & 0x1F;
   byte win_x_off    = 0;
   byte win_x_px     = 0;
   word bg_tile      = mem_direct_read(bg_map_loc
                                     + bg_map_off
                                     + bg_x_off);
   word win_tile     = mem_direct_read(win_map_loc
                                     + win_map_off
                                     + win_x_off);

   if (!tile_bank && bg_tile < 128) {
      bg_tile += 256;
   }
   if (!tile_bank && win_tile < 128) {
      win_tile += 256;
   }

   byte bg_xpx_off  = scroll_x & 0x07;
   byte bg_ypx_off  = (scroll_y + ppu_ly) & 0x07;
   byte win_xpx_off = 0;
   byte win_ypx_off = ppu_win_ly & 0x07;
   byte start_x_off = bg_xpx_off;
   byte outcol      = 0;

   for (int i = 0; i < 160; i++) {
      if ((mem_direct_read(LCD_CONTROL_ADDR) & 0x20)
        && ppu_ly >= win_y
        && i  >= win_x - 7
        && win_x < 166) {
         window = true;
      }

      word tile_addr = 0x8000;
      if (!window) {
         tile_addr += bg_tile  * 16 + bg_ypx_off  * 2;
      } else {
         tile_addr += win_tile * 16 + win_ypx_off * 2;
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

      bg_is_zero[i] = col == 0;
      outcol = ppu_pick_color(col, mem_direct_read(BG_PAL_ADDR));
      ppu_vram[vram_addr++] = outcol;
      ppu_vram[vram_addr++] = outcol;
      ppu_vram[vram_addr++] = outcol;

      if (!window) {
         bg_xpx_off++;
         if (bg_xpx_off >= 8) {
            bg_xpx_off = 0;
            bg_x_off++;
            bg_x_off &= 0x1F;
            bg_tile = mem_direct_read(bg_map_loc
                                    + bg_map_off
                                    + bg_x_off);
            if (!tile_bank && bg_tile < 128) {
               bg_tile += 256;
            }
         }
      } else {
         win_xpx_off++;
         if (win_xpx_off >= 8) {
            win_xpx_off = 0;
            win_x_off++;
            win_x_off &= 0x1F;
            win_tile = mem_direct_read(win_map_loc
                                     + win_map_off
                                     + win_x_off);
            if (!tile_bank && win_tile < 128) {
               win_tile += 256;
            }
         }
      }
   }

   if (window) {
      ppu_win_ly++;
   }

   // Check if sprites are enabled
   if(mem_direct_read(LCD_CONTROL_ADDR) & 0x02 == 0) {
      return;
   }

   // Begin sprite drawing
   bool big_sprites    = mem_direct_read(LCD_CONTROL_ADDR) & 0x04;
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
         word spr_addr = 0x8000
                       + spr_index * 16
                       + (spr_line & y_mask) * 2;
         byte shi = mem_direct_read(spr_addr + 1);
         byte slo = mem_direct_read(spr_addr);

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
            outcol = ppu_pick_color(scol,
                                    mem_direct_read(OBJ_PAL_ADDR + pal));
            
            if (draw_x + sx < 160 && draw_x + sx >= 0 && scol) {
               if (pri == 1 && !bg_is_zero[draw_x + sx]) {
                  continue;
               }
               int output_addr = (ppu_ly * 160 + draw_x + sx) * 3;
               ppu_vram[output_addr++] = outcol;
               ppu_vram[output_addr++] = outcol;
               ppu_vram[output_addr++] = outcol;
            }
         }
      }
   }
}

// TODO: 
byte ppu_pick_color(byte col, byte pal) {
   byte temp = (pal >> (col * 2)) & 0x03;
   switch (temp) {
      case 3: return COLOR_BLACK;
      case 2: return COLOR_DARK;
      case 1: return COLOR_LITE;
      case 0: return COLOR_WHITE;
   }
   return COLOR_WHITE;
}
