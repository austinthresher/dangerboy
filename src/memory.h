#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "datatypes.h"

#define ROM_NAME_ADDR 0x0134
#define CART_TYPE_ADDR 0x0147
#define ROM_SIZE_ADDR 0x0148
#define RAM_SIZE_ADDR 0x0149
#define SPRITE_RAM_START_ADDR 0xFE00
#define SPRITE_RAM_END_ADDR 0xFE9F
#define INPUT_REGISTER_ADDR 0xFF00
#define DIV_REGISTER_ADDR 0xFF04
#define TIMA_REGISTER_ADDR 0xFF05
#define TIMA_MODULO_ADDR 0xFF06
#define TIMER_CONTROL_ADDR 0xFF07
#define INT_FLAG_ADDR 0xFF0F
#define LCD_CONTROL_ADDR 0xFF40
#define LCD_STATUS_ADDR 0xFF41
#define LCD_SCY_ADDR 0xFF42
#define LCD_SCX_ADDR 0xFF43
#define LCD_SCANLINE_ADDR 0xFF44
// MBC1 could operate with two bank configurations.
// true  = 16 Mbit ROM 8  Kbyte RAM
// false = 4  Mbit ROM 32 Kbyte RAM
bool mem_mbc1_extended_mode; 
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
