#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "datatypes.h"

#define ROMNAME 0x0134
#define CARTTYPE 0x0147
#define ROMSIZE 0x0148
#define RAMSIZE 0x0149
#define OAMSTART 0xFE00
#define OAMEND 0xFEA0
#define JOYP 0xFF00
#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07
#define IF 0xFF0F
#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY 0xFF42
#define SCX 0xFF43
#define LY 0xFF44
#define LYC 0xFF45
#define DMA 0xFF46
#define BGPAL 0xFF47
#define OBJPAL 0xFF48
#define WINX 0xFF4B
#define WINY 0xFF4A
#define IE 0xFFFF

byte mem_dpad;
byte mem_buttons;

void mem_init();
void mem_free();
void mem_advance_time(cycle ticks);
void mem_load_image(char* fname);
void mem_print_rom_info();
void wbyte(word addr, byte val);
void wword(word addr, word val);
byte mem_get_current_rom_bank();
byte rbyte(word addr);
word rword(word addr);

// Direct memory access. Does not perform banking or register lookup.
// Used to get around normal memory access limitations (DMA transfers,
// registers that reset on write, etc).
void dwrite(word addr, byte val);
byte dread(word addr);

#endif
