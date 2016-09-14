#ifndef __GPU_H__
#define __GPU_H__

#include "datatypes.h"
#include "z80.h"

static const byte gpu_mode_hblank =
   0; // HBlank, takes 204 clocks, done 144 times per vblank
static const byte gpu_mode_vblank = 1; // VBlank, takes 4560 clocks
static const byte gpu_mode_scan_OAM =
   2; // Object attribute memory access, takes 80 clocks
static const byte gpu_mode_scan_VRAM =
   3; // Vram, takes 172 clocks. This is the end of a scanline
static const byte gpu_c_white = 0xFE;
static const byte gpu_c_lite  = 0xC6;
static const byte gpu_c_dark  = 0x7C;
static const byte gpu_c_black = 0x00;

byte  gpu_mode; // This mode needs to be written to 0xFF41
byte  gpu_scanline;
byte  gpu_window_scanline;
byte  gpu_window_y;
byte  gpu_last_scanline;
byte* gpu_vram;
bool  gpu_draw_frame;
bool  gpu_hit_stat_interrupt;
bool  gpu_blankscreen;
bool  gpu_ready_to_draw;
tick  gpu_timer;

void gpu_init();
void gpu_reset();
void gpu_execute_step(tick ticks);
void gpu_do_scanline();
void gpu_buildobj(word addr, byte val);
byte gpu_pick_color(byte col, byte pal);

#endif
