#include "lcd.h"
#include "debugger.h"

// Internal defines

#define BIT_LYCEQ (1 << 2)
#define BIT_HBLANK (1 << 3)
#define BIT_VBLANK (1 << 4)
#define BIT_OAM (1 << 5)
#define BIT_LYC (1 << 6)
// This macro switches between per-scanline and per-pixel rendering.
// TODO: Make this a runtime option.
//#define PER_PIXEL

// Internal variables
tick timer;
byte mode;
byte win_ly;
byte ly;
bool stat_vbl_on;
bool stat_hbl_on;
bool stat_oam_on;
bool stat_lyc_on;
bool stat_fired;
bool vblank_fired;
bool disabled;
int scroll_tick_mod;
int x_pixel;
int ignore_oams;

// Internal functions
void draw_pixel(int x, int y);
void draw_scanline();
void set_mode(byte new_mode);
void update_scroll_mod();
bool lyc();
byte get_color(byte col, byte pal);

// Function definitions

tick lcd_get_timer() { return timer; }
bool lcd_disabled() { return disabled; }
bool lyc() { return ly == mem_direct_read(LYC); }

bool fire_stat() {
   if (!stat_fired) {
      wbyte(IF, mem_direct_read(IF) | INT_STAT);
      stat_fired = true;
      return true;
   }
   return false;
}

// This fires the hardware VBLANK interrupt, not STAT
void fire_vblank() {
   lcd_draw = true;
   wbyte(IF, mem_direct_read(IF) | INT_VBLANK);
}

void lcd_init() {
   disabled     = false;
   lcd_draw     = false;
   x_pixel      = 0;
   vblank_fired = false;
   ignore_oams  = 0;
   stat_fired   = false;

   stat_vbl_on = false;
   stat_oam_on = false;
   stat_lyc_on = false;
   lcd_reset();
}

void lcd_reset() {
   mode   = LCD_MODE_OAM;
   ly     = 0;
   win_ly = 0;
   timer  = 0;
}

void try_fire_oam() {
   if (stat_oam_on) {
      if (ignore_oams == 0) {
         // STAT documentation indicates that the OAM after a STAT VBLANK
         // should be skipped, but this causes mooneye intr_1_2_timing to
         // fail, which is verified on real hardware.
         //    if (!(ly == 0 && vblank_fired)) {
         fire_stat();
      }
   }
   if (ignore_oams) {
      ignore_oams--;
   }
   vblank_fired = false;
}

void try_fire_hblank() {
   if (stat_hbl_on) {
      if (fire_stat()) {
         ignore_oams = 1;
      }
   }
}

void try_fire_vblank() {
   if (stat_vbl_on) {
      if (fire_stat()) {
         vblank_fired = true;
      }
   } else if (ly == 144) {
      // OAM interrupt still fires on ly == 144,
      // but only if STAT VBLANK interrupt did not
      try_fire_oam();
   }
}

void try_fire_lyc() {
   if (stat_lyc_on && lyc()) {
      if (!vblank_fired) {
         if (fire_stat()) {
            if (ly < 0x90) {
               ignore_oams = 1;
            }
         }
      }
   }
}

// This used to do more, but now it just outputs
// debug messages. Still useful.
void set_mode(byte new_mode) {
   if (mode != new_mode) {
      switch (new_mode) {
         case LCD_MODE_VBLANK: debugger_log("STAT mode switch: VBLANK"); break;
         case LCD_MODE_HBLANK: debugger_log("STAT mode switch: HBLANK"); break;
         case LCD_MODE_OAM: debugger_log("STAT mode switch: OAM"); break;
         case LCD_MODE_VRAM: debugger_log("STAT mode switch: VRAM"); break;
      }
   }
   mode = new_mode;
}

byte lcd_reg_read(word addr) {
   switch (addr) {
      case LY: return ly;
      case STAT:
         return 0x80 | (stat_lyc_on << 6) | (stat_oam_on << 5)
                | (stat_vbl_on << 4) | (stat_hbl_on << 3) | (lyc() << 2)
                | (mode & 3);
      default: break;
   }
   return mem_direct_read(addr);
}

void lcd_reg_write(word addr, byte val) {
   switch (addr) {
      case LY:
         ly     = 0;
         win_ly = 0;
         debugger_notify_mem_write(LY, 0);
         break;
      case LCDC:
         mem_direct_write(LCDC, val);
         if (val & 0x80) {
            if (disabled) {
               set_mode(LCD_MODE_HBLANK);
               timer = 200 - 84; // This is supposed to last the same length as
                                 // OAM mode
                                 //               timer = 0;
               //               set_mode(LCD_MODE_OAM);
               try_fire_lyc();
            }
            disabled = false;
         } else {
            if (!disabled) {
               lcd_draw = true;
               disabled = true;
               ly       = 0;
               win_ly   = 0;
               debugger_notify_mem_write(LY, 0);
               set_mode(LCD_MODE_HBLANK);
            }
         }
         break;
      case STAT:
         stat_vbl_on = val & BIT_VBLANK;
         stat_hbl_on = val & BIT_HBLANK;
         stat_oam_on = val & BIT_OAM;
         stat_lyc_on = val & BIT_LYC;
         break;
      default: mem_direct_write(addr, val);
   }
}

// Based on Mooneye's implementation
void update_scroll_mod() {
   byte scx        = mem_direct_read(SCX) % 8;
   scroll_tick_mod = 0;
   if (scx > 4) {
      scroll_tick_mod = 8;
   } else if (scx > 0) {
      scroll_tick_mod = 4;
   }
}

void lcd_advance_time(tick ticks) {
   stat_fired = false;

   if (disabled) {
      return;
   }

   int vram_length = 172 + scroll_tick_mod;
   tick old_timer  = timer;
   byte stat_reg   = mem_direct_read(STAT);

   timer += ticks;

   switch (mode) {
      case LCD_MODE_OAM:
         if (timer >= 84) {
            timer -= 84;
            update_scroll_mod();
            set_mode(LCD_MODE_VRAM);
         }
         break;

      case LCD_MODE_VRAM:

#ifdef PER_PIXEL
         // Draw our current scanline one pixel at a time
         while (x_pixel < timer - 12 && x_pixel < 160) {
            draw_pixel(x_pixel++, ly);
         }
#else
         if (timer >= vram_length && x_pixel < 160) {
            draw_scanline();
            x_pixel = 160;
         }
#endif
         // According to Mooneye tests, HBLANK STAT interrupt is 4 cycles early
         if (old_timer < vram_length - 4 && timer >= vram_length - 4) {
            try_fire_hblank();
         }
         if (timer >= vram_length) {
            timer -= vram_length;
            set_mode(LCD_MODE_HBLANK);
         }

         break;

      case LCD_MODE_HBLANK:
         if (timer >= 200 - scroll_tick_mod) {
            timer -= 200 - scroll_tick_mod;
            x_pixel = 0;
            ly++;
            debugger_notify_mem_write(LY, ly);
            try_fire_lyc();
            if (ly >= 144) {
               fire_vblank();
               set_mode(LCD_MODE_VBLANK);
               try_fire_vblank();
            } else {
               set_mode(LCD_MODE_OAM);
               try_fire_oam();
            }
         }
         break;
      case LCD_MODE_VBLANK:
         // LY = 99 only lasts for 56 cycles,
         // which makes LY == 0 last for 856
         if (ly >= 153 && timer >= 56) {
            ly     = 0;
            win_ly = 0;
            try_fire_lyc();
            // After LYC == 0 at 99, the next 2 OAMs are ignored
            if (ignore_oams == 1) {
               ignore_oams = 2;
            }
         } else if (timer >= 456) {
            timer -= 456;
            ly++;
            if (ly == 1) {
               ly     = 0;
               win_ly = 0;
               set_mode(LCD_MODE_OAM);
               try_fire_oam();
            }
            // LY == 0 happens during VBLANK for LY == 153
            if (ly != 0) {
               try_fire_lyc();
            }
         }
         break;
   }
}

void draw_pixel(int x, int y) {
   if (x < 0 || x >= 160 || y < 0 || y >= 144) {
      return;
   }

   // TODO: These should be changed on write to LCDC and stored.
   byte lcdc       = rbyte(LCDC);
   bool wn_tilemap = lcdc & (1 << 6);
   bool wn_enabled = lcdc & (1 << 5);
   bool tile_bank  = lcdc & (1 << 4);
   bool bg_tilemap = lcdc & (1 << 3);
   bool sp_size    = lcdc & (1 << 2);
   bool sp_enabled = lcdc & (1 << 1);
   bool bg_enabled = lcdc & (1 << 0);

   int scroll_x = mem_direct_read(SCX);
   int scroll_y = mem_direct_read(SCY);
   // TODO: Draw the window here too
   int wn_x_pos = mem_direct_read(WINX) - 7;
   int wn_y_pos = mem_direct_read(WINY);

   bool bg_in_front = true;

   bool skip_bg = !bg_enabled;
   if (wn_enabled && wn_x_pos < 166 && y >= wn_y_pos && x >= wn_x_pos) {
      skip_bg = true;
   }
   if (!skip_bg) {
      // The background tilemap is 256x256. Here we calculate
      // which pixel we are in this map, wrapped.
      int tilemap_x = (x + scroll_x) % 256;
      int tilemap_y = (y + scroll_y) % 256;

      // Divide by 8 to find tile coordinates
      int tilemap_tile_x = tilemap_x / 8; // >> 3;
      int tilemap_tile_y = tilemap_y / 8; // >> 3;

      // The tilemap is a 32 x 32 grid. Get the index
      // of the tile we're drawing.
      int tile_index = tilemap_tile_y * 32 + tilemap_tile_x;

      tile_index =
            mem_direct_read(0x9800 + (bg_tilemap ? 0x400 : 0) + tile_index);

      int tile_addr = 0x8000;
      if (tile_index < 128) {
         if (tile_bank) {
            tile_addr += tile_index * 16;
         } else {
            tile_addr += 0x1000 + tile_index * 16;
         }
      } else {
         tile_addr += 0x800 + (tile_index - 128) * 16;
      }

      // We now have the address of the tile we're drawing.
      // Tiles are stored in 16 bytes each, like so:
      //    [Row 1 Lo Bits] [Row 1 Hi Bits] [Row 2 Lo Bits]...
      int tile_px_x = tilemap_x & 7;
      int tile_px_y = tilemap_y & 7;
      int row_byte  = tile_addr + tile_px_y * 2;
      int pal_index =
            (!!(mem_direct_read(row_byte) & (0x80 >> tile_px_x)))
            | (!!(mem_direct_read(row_byte + 1) & (0x80 >> tile_px_x)) << 1);

      if (pal_index == 0) {
         bg_in_front = false;
      }
      byte color = get_color(pal_index, mem_direct_read(BGPAL));

      int vram_addr         = (y * 160 + x) * 3;
      lcd_vram[vram_addr++] = color;
      lcd_vram[vram_addr++] = color;
      lcd_vram[vram_addr++] = color;
   } else if (!bg_enabled && !wn_enabled) {
      int vram_addr         = (y * 160 + x) * 3;
      lcd_vram[vram_addr++] = LCD_WHITE;
      lcd_vram[vram_addr++] = LCD_WHITE;
      lcd_vram[vram_addr++] = LCD_WHITE;
   }

   if (sp_enabled) {
      int sp_height = sp_size ? 16 : 8;
      for (int s = 0; s < 40; ++s) {
         int sp_y = mem_direct_read(OAM + s * 4) - 16;
         int sp_x = mem_direct_read(OAM + s * 4 + 1) - 8;
         if (sp_x == 0 || sp_y == 0) {
            continue;
         }
         if (sp_x + 8 <= x || sp_x > x || sp_y > y || sp_y + sp_height <= y) {
            continue;
         }

         byte tile = mem_direct_read(OAM + s * 4 + 2);
         byte attr = mem_direct_read(OAM + s * 4 + 3);

         if ((attr & 0x80) && bg_in_front) {
            continue;
         }

         // Y Flip
         int sp_row = y - sp_y;
         if (attr & 0x40) {
            sp_row = sp_height - 1 - sp_row;
         }
         if (sp_size) {
            tile &= 0xFE;
         }

         // X Flip
         int tile_px_x = (x - sp_x) & 7;
         if (attr & 0x20) {
            tile_px_x = 7 - tile_px_x;
         }

         byte tile_index_mask = sp_height - 1;

         int row_byte = 0x8000 + tile * 16 + (sp_row & tile_index_mask) * 2;
         int pal_index =
               (!!(mem_direct_read(row_byte) & (0x80 >> tile_px_x)))
               | (!!(mem_direct_read(row_byte + 1) & (0x80 >> tile_px_x)) << 1);

         if (pal_index == 0) {
            continue;
         }

         byte color =
               get_color(pal_index, mem_direct_read(OBJPAL + !!(attr & 0x10)));

         int vram_addr         = (y * 160 + x) * 3;
         lcd_vram[vram_addr++] = color;
         lcd_vram[vram_addr++] = color;
         lcd_vram[vram_addr++] = color;
      }
   }
}

void draw_scanline() {
   if (ly > 143) {
      return;
   }
   bool bg_is_zero[160];
   bool window   = false;
   int vram_addr = ly * 160 * 3;
   byte win_x    = mem_direct_read(WINX);
   if (ly == 0) {
      win_ly = mem_direct_read(WINY);
   }
   bool bg_enabled   = mem_direct_read(LCDC) & 0x01;
   bool win_tile_map = mem_direct_read(LCDC) & 0x40;
   bool bg_tile_map  = mem_direct_read(LCDC) & 0x08;
   bool tile_bank    = mem_direct_read(LCDC) & 0x10;
   byte scroll_x     = mem_direct_read(SCX);
   byte scroll_y     = mem_direct_read(SCY);
   word bg_map_loc   = 0x9800 + (bg_tile_map ? 0x400 : 0);
   word win_map_loc  = 0x9800 + (win_tile_map ? 0x400 : 0);
   word bg_map_off   = (((ly + scroll_y) & 0xFF) >> 3) * 32;
   word win_map_off  = ((win_ly & 0xFF) >> 3) * 32;
   byte bg_x_off     = (scroll_x >> 3) & 0x1F;
   byte win_x_off    = 0; // What is this supposed to be set to?
   byte bg_tile      = mem_direct_read(bg_map_loc + bg_map_off + bg_x_off);
   byte win_tile     = mem_direct_read(win_map_loc + win_map_off + win_x_off);
   byte bg_xpx_off   = scroll_x & 0x07;
   byte bg_ypx_off   = (scroll_y + ly) & 0x07;
   byte win_ypx_off  = win_ly & 0x07;
   byte start_x_off  = bg_xpx_off;
   byte win_xpx_off  = 0;
   byte win_x_px     = 0;
   byte outcol       = 0;

   for (int i = 0; i < 160; i++) {
      if ((mem_direct_read(LCDC) & 0x20) && ly >= win_ly && i >= win_x - 7
            && win_x < 166) {
         window = true;
      }

      byte tile_index = window ? win_tile : bg_tile;
      int tile_addr   = 0x8000;
      if (tile_index < 128) {
         if (tile_bank) {
            tile_addr += tile_index * 16;
         } else {
            tile_addr += 0x1000 + tile_index * 16;
         }
      } else {
         tile_addr += 0x800 + (tile_index - 128) * 16;
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
         bg_is_zero[i] = col == 0;
         outcol        = get_color(col, mem_direct_read(BGPAL));
         if (vram_addr + 2 < 160 * 144 * 3) {
            lcd_vram[vram_addr++] = outcol;
            lcd_vram[vram_addr++] = outcol;
            lcd_vram[vram_addr++] = outcol;
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
               win_tile =
                     mem_direct_read(win_map_loc + win_map_off + win_x_off);
            }
         }
      } else {
         bg_is_zero[i]         = true;
         lcd_vram[vram_addr++] = LCD_WHITE;
         lcd_vram[vram_addr++] = LCD_WHITE;
         lcd_vram[vram_addr++] = LCD_WHITE;
      }
   }

   if (window) {
      win_ly++;
   }

   // Check if sprites are enabled
   if (!(mem_direct_read(LCDC) & 0x02)) {
      return;
   }

   // Begin sprite drawing
   bool big_sprites = mem_direct_read(LCDC) & 0x04;
   for (int spr = 0; spr < 40; spr++) {
      byte y     = mem_direct_read(OAM + spr * 4);
      byte x     = mem_direct_read(OAM + spr * 4 + 1);
      byte tile  = mem_direct_read(OAM + spr * 4 + 2);
      byte attr  = mem_direct_read(OAM + spr * 4 + 3);
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

      if (draw_y <= ly && draw_y + height > ly) {
         byte spr_index = tile;
         // In 8x16 mode, the least significant bit is ignored
         if (big_sprites) {
            spr_index &= 0xFE;
         }

         byte spr_line = ly - draw_y;
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
            outcol = get_color(scol, mem_direct_read(OBJPAL + pal));

            if (draw_x + sx < 160 && draw_x + sx >= 0 && scol) {
               if (pri == 1 && !bg_is_zero[draw_x + sx]) {
                  continue;
               }
               int output_addr = (ly * 160 + draw_x + sx) * 3;
               if (output_addr + 2 < 160 * 144 * 3) {
                  lcd_vram[output_addr++] = outcol;
                  lcd_vram[output_addr++] = outcol;
                  lcd_vram[output_addr++] = outcol;
               }
            }
         }
      }
   }
}

byte get_color(byte col, byte pal) {
   byte temp = (pal >> (col * 2)) & 0x03;
   switch (temp) {
      case 3: return LCD_BLACK;
      case 2: return LCD_DARK;
      case 1: return LCD_LITE;
      case 0: return LCD_WHITE;
   }
   return LCD_WHITE;
}
