#ifndef __PPU_H__
#define __PPU_H__

#include "cpu.h"
#include "datatypes.h"

// HBlank. Lasts 200 ticks according to mooneye
#define PPU_MODE_HBLANK 0

// VBlank. Lasts 4560 ticks.
#define PPU_MODE_VBLANK 1

// Object attribute memory access. Lasts 84 ticks according to mooneye
#define PPU_MODE_SCAN_OAM 2

// VRAM access. Lasts 172 ticks.
#define PPU_MODE_SCAN_VRAM 3

#define LCD_WHITE 0xFE
#define LCD_LITE 0xC6
#define LCD_DARK 0x7C
#define LCD_BLACK 0x00

byte ppu_vram[160 * 144 * 3];
byte ppu_ly;
byte ppu_win_ly;
bool ppu_draw;
bool lcd_disable;

void ppu_init();
void ppu_reset();
void ppu_advance_time(tick ticks);
void ppu_do_scanline();
void ppu_buildobj(word addr, byte val);
byte ppu_pick_color(byte col, byte pal);
void ppu_update_register(word addr, byte val);
tick ppu_get_timer();

#endif
