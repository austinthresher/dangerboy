#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "datatypes.h"

bool mem_MBC1_16MROM_8KRAM_mode;
bool mem_ram_bank_locked;
bool mem_need_reset_scanline;
byte mem_bank_mode;
byte mem_rom_bank_count;
byte mem_ram_bank_count;
byte mem_current_rom_bank;
byte mem_current_ram_bank;
byte mem_dpad;
byte mem_buttons;
byte mem_input_last_write;

byte* mem_ram;
byte* mem_rom;
byte* mem_ram_bank;

char* mem_rom_name;

void mem_init();
void mem_load_image(char* fname);
void mem_get_rom_info();
void mem_wb(word addr, byte val);
void mem_ww(word addr, word val);
void mem_direct_write(word addr, byte val);
byte mem_rb(word addr);
byte mem_get_current_rom_bank();
byte mem_direct_read(word addr);
word mem_rw(word addr);

#endif
