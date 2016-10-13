#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "defines.h"

// Joypad interface.
// TODO: Move this to a different file,
// maybe a gb.c / h to replace a lot of the
// code in main.c?

typedef enum button_ { A = 1, B = 2, SELECT = 4, START = 8 } button;
typedef enum dpad_ { RIGHT = 1, LEFT = 2, UP = 4, DOWN = 8 } dpad;

void press_button(button but);
void press_dpad(dpad dir);
void release_button(button but);
void release_dpad(dpad dir);

void mem_init();
void mem_advance_time(cycle ticks);
void mem_load_image(char* fname);
void mem_print_rom_info();
void wbyte(word addr, byte val);
void wword(word addr, word val);
byte rbyte(word addr);
word rword(word addr);

// Direct memory access. Does not perform banking or register lookup.
// Used to get around normal memory access limitations (DMA transfers,
// registers that reset on write, etc).
void dwrite(word addr, byte val);
byte dread(word addr);

#endif
