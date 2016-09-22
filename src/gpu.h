#ifndef __GPU_H__
#define __GPU_H__

#include "datatypes.h"
#include "z80.h"

// HBlank. Lasts 204 ticks.
#define GPU_MODE_HBLANK 0

// VBlank. Lasts 4560 ticks.
#define GPU_MODE_VBLANK 1

// Object attribute memory access. Lasts 80 ticks
#define GPU_MODE_SCAN_OAM 2

// VRAM access. Lasts 172 ticks. 
#define GPU_MODE_SCAN_VRAM 3

#define COLOR_WHITE 0xFE
#define COLOR_LITE 0xC6
#define COLOR_DARK 0x7C
#define COLOR_BLACK 0x00

byte  gpu_ly;
byte  gpu_win_ly;
byte* gpu_vram;
bool  gpu_ready_to_draw;

void gpu_init();
void gpu_reset();
void gpu_advance_time(tick ticks);
void gpu_do_scanline();
void gpu_buildobj(word addr, byte val);
byte gpu_pick_color(byte col, byte pal);

#endif
