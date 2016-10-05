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
#define TIMA_ADDR 0xFF05
#define TMA_ADDR 0xFF06
#define TIMER_CONTROL_ADDR 0xFF07
#define INT_FLAG_ADDR 0xFF0F
#define LCD_CONTROL_ADDR 0xFF40
#define LCD_STATUS_ADDR 0xFF41
#define LCD_SCY_ADDR 0xFF42
#define LCD_SCX_ADDR 0xFF43
#define LCD_LINE_Y_ADDR 0xFF44
#define LCD_LINE_Y_C_ADDR 0xFF45
#define OAM_DMA_ADDR 0xFF46
#define BG_PAL_ADDR 0xFF47
#define OBJ_PAL_ADDR 0xFF48
#define WIN_X_ADDR 0xFF4B
#define WIN_Y_ADDR 0xFF4A
#define INT_ENABLED_ADDR 0xFFFF

typedef enum mbc_type_ { NONE = 0, MBC1 = 1, MBC2 = 2, MBC3 = 3 } mbc_type;

typedef enum mbc_bankmode_ { ROM16_RAM8 = 0, ROM4_RAM32 = 1 } mbc_bankmode;

mbc_type mem_mbc_type;
mbc_bankmode mem_mbc_bankmode;

bool mem_ram_bank_locked;
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

char mem_rom_name[16];

void mem_init();
void mem_free();
void mem_advance_time(tick ticks);
void mem_load_image(char* fname);
void mem_print_rom_info();
void mem_wb(word addr, byte val);
void mem_ww(word addr, word val);
void mem_direct_write(word addr, byte val);
void mem_enable_debug_access(bool enabled);
byte mem_rb(word addr);
byte mem_get_current_rom_bank();
byte mem_direct_read(word addr);
word mem_rw(word addr);

#endif
