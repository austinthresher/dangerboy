#ifndef __PPU_H__
#define __PPU_H__

#include "cpu.h"
#include "datatypes.h"

// HBlank. Lasts 204 ticks.
#define PPU_MODE_HBLANK 0

// VBlank. Lasts 4560 ticks.
#define PPU_MODE_VBLANK 1

// Object attribute memory access. Lasts 80 ticks
#define PPU_MODE_SCAN_OAM 2

// VRAM access. Lasts 172 ticks.
#define PPU_MODE_SCAN_VRAM 3

#define COLOR_WHITE 0xFE
#define COLOR_LITE 0xC6
#define COLOR_DARK 0x7C
#define COLOR_BLACK 0x00

byte ppu_ly;
byte ppu_win_ly;
byte* ppu_vram;
bool ppu_ready_to_draw;
bool lcd_disable;

void ppu_init();
void ppu_reset();
void ppu_advance_time(tick ticks);
void ppu_do_scanline();
void ppu_buildobj(word addr, byte val);
byte ppu_pick_color(byte col, byte pal);
void ppu_update_register(word addr, byte val);
#endif
