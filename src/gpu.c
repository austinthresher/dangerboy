#include "gpu.h"

void gpu_init() {
   gpu_mode               = GPU_MODE_SCAN_OAM;
   gpu_timer              = 0;
   gpu_scanline           = 0;
   gpu_blankscreen        = false;
   gpu_ready_to_draw      = false;
   gpu_hit_stat_interrupt = false;
   gpu_vram               = NULL;
   gpu_window_scanline    = 0;
   gpu_window_y           = 0;
   gpu_reset();
}

void gpu_reset() {
   if (gpu_vram != NULL) {
      free(gpu_vram);
   }

   // 4 bytes per pixel
   gpu_vram = (byte*)calloc(160 * 144, 4);

   for (int i = 0, n = 0; i < 40; i++, n += 4) {
      mem_wb(0xFE00 + n + 0, 0);
      mem_wb(0xFE00 + n + 1, 0);
      mem_wb(0xFE00 + n + 2, 0);
      mem_wb(0xFE00 + n + 3, 0);
   }
}

void gpu_update_stat() {
   
   // If the LCD is disabled, freeze our status
   // just before we jump back to OAM mode. We
   // Stay in VBLANK until the LCD is enabled.
   if ((mem_rb(LCD_CONTROL_ADDR) & 0x80) == 0) {
      if (!gpu_blankscreen) {
         gpu_blankscreen = true;
         gpu_ready_to_draw = true;
      }
      gpu_timer = 456;
      gpu_mode = GPU_MODE_VBLANK;
      gpu_scanline = 153;
      mem_direct_write(LCD_LINE_Y_ADDR, 0);
      byte stat = mem_rb(LCD_STATUS_ADDR) & 0xFC;
      stat |= 1;
      mem_wb(LCD_STATUS_ADDR, stat);
      return;
   }

   gpu_blankscreen = false;

   // Writing here normally resets it, so we set it directly
   mem_direct_write(LCD_LINE_Y_ADDR, gpu_scanline);

   // If lyc == scanline and bit 6 of stat is set, interrupt
   byte lyc = mem_rb(LCD_LINE_Y_C_ADDR);

   // Bits 0-2 indicate the gpu mode.
   // Higher bits are interrupt enable flags.
   byte new_stat = (gpu_scanline == lyc ? 0x04 : 0) | (gpu_mode & 0x03);
   byte stat_reg = mem_rb(LCD_STATUS_ADDR);
   mem_wb(LCD_STATUS_ADDR, (stat_reg & 0xF8) | (new_stat & 0x07));

   // The STAT interrupt has 4 different modes, based on bits 3-6
   if ((gpu_mode == GPU_MODE_HBLANK   && (stat_reg & 0x08))
    || (gpu_mode == GPU_MODE_VBLANK   && (stat_reg & 0x10))
    || (gpu_mode == GPU_MODE_SCAN_OAM && (stat_reg & 0x20))
    || (gpu_scanline == lyc           && (stat_reg & 0x40))) {
      mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_STAT);
   } else if (gpu_scanline == 144 && gpu_mode == GPU_MODE_VBLANK) {
      // The OAM interrupt can still fire on scanline 144. This only
      // happens if the VBlank interrupt did not fire.
      if (stat_reg & 0x20) {
         mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_STAT);
      }
   }
}

void gpu_execute_step(tick ticks) {
   gpu_timer += ticks;

   switch (gpu_mode) {
      case GPU_MODE_SCAN_OAM:
         if (gpu_timer >= 80) {
            gpu_mode = GPU_MODE_SCAN_VRAM;
            gpu_timer = 0;
            gpu_update_stat();
         }
         break;
      case GPU_MODE_SCAN_VRAM:
         if (gpu_timer >= 172) {
            gpu_mode = GPU_MODE_HBLANK;
            gpu_timer = 0;
            gpu_do_scanline();
            gpu_update_stat();         
         }
         break;
      case GPU_MODE_HBLANK:
         if (gpu_timer >= 204) {
            gpu_timer = 0;
            gpu_scanline++;
            if (gpu_scanline > 143) {
               gpu_mode = GPU_MODE_VBLANK;
               gpu_ready_to_draw = true;
               mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_VBLANK);
            }
            else {
               gpu_mode = GPU_MODE_SCAN_OAM;
            }
            gpu_update_stat();
         }
         break;
      case GPU_MODE_VBLANK:
         if (gpu_timer >= 456) {
            gpu_timer = 0;
            gpu_scanline++;
            if (gpu_scanline > 153) {
               gpu_mode = GPU_MODE_SCAN_OAM;
               gpu_scanline = 0;
               gpu_window_scanline = 0;
            }
            gpu_update_stat();
         }
         break;
   }

   // TODO:
   //emutalk.net/threads/41525-Game-Boy/page104
   // "It turns out the mode cycles through 2, 3, 0, 1
   //  during vblank, not 0, 2, 3"

   // After reading more, it looks like VBLANK should start
   // on line 143, not 144. Also, the order of GPU modes is wrong here:
   // It should go OAM / VRAM / HBLANK. This is probably causing problems.

   // Now lets update our memory registers
   // TODO: 0xFF40 - LCD and GPU misc stuff
}

void gpu_do_scanline() {
   bool using_window = false;
   int  output_addr  = gpu_scanline * 160 * 4;

   if (gpu_blankscreen) {
      for (int i = 0; i < 160; i++) {
         gpu_vram[output_addr++] = 0xCC;
         gpu_vram[output_addr++] = 0xFF;
         gpu_vram[output_addr++] = 0xCC;
         gpu_vram[output_addr++] = 0xFF;
      }
      return;
   }

   byte gpu_window_x = mem_rb(0xFF4B);

   if (gpu_scanline == 0) {
      gpu_window_y = mem_rb(0xFF4A);
   }

   byte scrollX            = mem_rb(LCD_SCX_ADDR);
   byte scrollY            = mem_rb(LCD_SCY_ADDR);
   bool which_win_tile_map = (mem_rb(LCD_CONTROL_ADDR) & 0x40) != 0;
   bool which_bg_tile_map  = (mem_rb(LCD_CONTROL_ADDR) & 0x08) != 0;
   bool which_tile_data    = (mem_rb(LCD_CONTROL_ADDR) & 0x10) == 0;

   word bg_tile_map_location = 0x9800;
   if (which_bg_tile_map) {
      bg_tile_map_location += 0x0400;
   }

   word win_tile_map_location = 0x9800;
   if (which_win_tile_map) {
      win_tile_map_location += 0x0400;
   }

   word bg_tile_map_offset  = (((gpu_scanline + scrollY) & 0xFF) >> 3) * 32;
   word win_tile_map_offset = ((gpu_window_scanline & 0xFF) >> 3) * 32;
   byte bg_x_tile_offset    = (scrollX >> 3) & 0x1F;
   byte win_x_tile_offset   = 0;
   byte window_x_px         = 0;
   word bg_current_tile     = mem_direct_read(
      bg_tile_map_location + bg_tile_map_offset + bg_x_tile_offset);

   if (which_tile_data && bg_current_tile < 128) {
      bg_current_tile += 256;
   }

   word win_current_tile = mem_direct_read(
      win_tile_map_location + win_tile_map_offset + win_x_tile_offset);

   if (which_tile_data && win_current_tile < 128) {
      win_current_tile += 256;
   }

   byte bg_x_px_offset  = scrollX & 0x07;
   byte bg_y_px_offset  = (scrollY + gpu_scanline) & 0x07;
   byte win_x_px_offset = 0;
   byte win_y_px_offset = gpu_window_scanline & 0x07;
   byte start_x_off     = bg_x_px_offset;
   byte outcol          = 0;

   for (int i = 0; i < 160; i++) {
      if ((mem_rb(0xFF40) & 0x20) != 0 && gpu_scanline >= gpu_window_y &&
          i >= gpu_window_x - 7 && gpu_window_x < 166) {
         using_window = true;
      }

      word tempaddr = 0x8000;
      if (using_window == false) {
         tempaddr += bg_current_tile * 16 + bg_y_px_offset * 2;
      } else {
         tempaddr += win_current_tile * 16 + win_y_px_offset * 2;
      }

      byte hi   = mem_direct_read(tempaddr + 1);
      byte lo   = mem_direct_read(tempaddr);
      byte mask = 1;

      if (using_window == false) {
         mask <<= (7 - ((i + start_x_off) & 0x07));
      } else {
         mask <<= (7 - ((window_x_px++) & 0x07));
      }

      byte col = 0;
      if (hi & mask) {
         col = 2;
      }
      if (lo & mask) {
         col += 1;
      }

      outcol = gpu_pick_color(col, mem_direct_read(0xFF47));

      if (using_window == false) {
         gpu_vram[output_addr++] = outcol;
         gpu_vram[output_addr++] = outcol;
         gpu_vram[output_addr++] = outcol;
         gpu_vram[output_addr++] = 0xFF; // Alpha channel

         bg_x_px_offset++;

         if (bg_x_px_offset >= 8) {
            bg_x_px_offset = 0;
            bg_x_tile_offset++;
            bg_x_tile_offset &= 0x1F;
            bg_current_tile = mem_direct_read(
               bg_tile_map_location + bg_tile_map_offset + bg_x_tile_offset);
            if (which_tile_data && bg_current_tile < 128) {
               bg_current_tile += 256;
            }
         }
      } else {
         gpu_vram[output_addr++] = outcol;
         gpu_vram[output_addr++] = outcol;
         gpu_vram[output_addr++] = outcol;
         gpu_vram[output_addr++] = 0xFF; // Alpha channel
         win_x_px_offset++;

         if (win_x_px_offset >= 8) {
            win_x_px_offset = 0;
            win_x_tile_offset++;
            win_x_tile_offset &= 0x1F;
            win_current_tile = mem_direct_read(
               win_tile_map_location + win_tile_map_offset + win_x_tile_offset);
            if (which_tile_data && win_current_tile < 128) {
               win_current_tile += 256;
            }
         }
      }
   }

   if (using_window) {
      gpu_window_scanline++;
   }

   bool big_sprites = (mem_direct_read(0xFF40) & 0x04) == 0x04;

   for (int spr = 0; spr < 40; spr++) {
      byte y        = mem_direct_read(0xFE00 + spr * 4);
      byte x        = mem_direct_read(0xFE00 + 1 + spr * 4);
      byte tile     = mem_direct_read(0xFE00 + 2 + spr * 4);
      byte val      = mem_direct_read(0xFE00 + 3 + spr * 4);
      byte palette  = (val & 0x10) ? 1 : 0;
      bool xflip    = (val & 0x20) ? true : false;
      bool yflip    = (val & 0x40) ? true : false;
      byte priority = (val & 0x80) ? 1 : 0;

      if (x == 0 && y == 0) {
         continue;
      }

      x -= 8;
      y -= 16;

      if (y <= gpu_scanline && y + (big_sprites ? 16 : 8) > gpu_scanline) {
         byte temptile = tile;
         if (mem_rb(0xFF40) & 0x04) {
            temptile &= 0xFE;
         }
         outcol = 0;
         word stempaddr;
         byte y_mask = 0x07;

         if (big_sprites) {
            y_mask = 0x0F;
         }
         if (!yflip) {
            stempaddr =
               0x8000 + temptile * 16 + ((gpu_scanline - y) & y_mask) * 2;
         } else {
            // TODO: Why is this 14 instead of 16?
            stempaddr =
               0x8000 + temptile * 16 +
               ((big_sprites ? 32 : 14) - ((gpu_scanline - y) & y_mask) * 2);
            // TODO: There's an off by 1 error somewhere with big sprites
         }

         byte shi = mem_direct_read(stempaddr + 1);
         byte slo = mem_direct_read(stempaddr);

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
            byte color =
               gpu_pick_color(scol, mem_direct_read(0xFF48 + palette));

            // TODO: This doesn't check priority with background. Fix.
            if ((x + sx) < 160 && scol) {
               outcol          = color;
               int output_addr = (gpu_scanline * 160 + x + sx) * 4;
               if (!gpu_blankscreen) {
                  gpu_vram[output_addr++] = outcol; // * 0.9;
                  gpu_vram[output_addr++] = outcol;
                  gpu_vram[output_addr++] = outcol; // * 0.6;
                  gpu_vram[output_addr++] = 0xFF;
               } else {
                  gpu_vram[output_addr++] = 0xFF;
                  gpu_vram[output_addr++] = 0xFF;
                  gpu_vram[output_addr++] = 0xFF;
                  gpu_vram[output_addr++] = 0xFF;
               }
            }
         }
      }
   }
}

byte gpu_pick_color(byte col, byte pal) {
   byte outcol = 0;
   if (col == 0x03) {
      byte temp = (pal >> 6) & 0x03;
      if (temp == 3) {
         outcol = COLOR_BLACK;
      } else if (temp == 2) {
         outcol = COLOR_DARK;
      } else if (temp == 1) {
         outcol = COLOR_LITE;
      } else if (temp == 0) {
         outcol = COLOR_WHITE;
      }
   } else if (col == 0x02) {
      byte temp = (pal >> 4) & 0x03;
      if (temp == 3) {
         outcol = COLOR_BLACK;
      } else if (temp == 2) {
         outcol = COLOR_DARK;
      } else if (temp == 1) {
         outcol = COLOR_LITE;
      } else if (temp == 0) {
         outcol = COLOR_WHITE;
      }
   } else if (col == 0x01) {
      byte temp = (pal >> 2) & 0x03;
      if (temp == 3) {
         outcol = COLOR_BLACK;
      } else if (temp == 2) {
         outcol = COLOR_DARK;
      } else if (temp == 1) {
         outcol = COLOR_LITE;
      } else if (temp == 0) {
         outcol = COLOR_WHITE;
      }
   } else if (col == 0x00) {
      byte temp = pal & 0x03;
      if (temp == 3) {
         outcol = COLOR_BLACK;
      } else if (temp == 2) {
         outcol = COLOR_DARK;
      } else if (temp == 1) {
         outcol = COLOR_LITE;
      } else if (temp == 0) {
         outcol = COLOR_WHITE;
      }
   }
   return outcol;
}
