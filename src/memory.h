#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "datatypes.h"

#define ROMNAME 0x0134
#define CARTTYPE 0x0147
#define ROMSIZE 0x0148
#define RAMSIZE 0x0149
#define OAM 0xFE00
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
#define DMASTART 0xFF46
#define BGPAL 0xFF47
#define OBJPAL 0xFF48
#define WINX 0xFF4B
#define WINY 0xFF4A
#define IE 0xFFFF

typedef enum mbc_type_ { NONE = 0, MBC1 = 1, MBC2 = 2, MBC3 = 3 } mbc_type;
typedef enum mbc_bankmode_ { ROM16_RAM8 = 0, ROM4_RAM32 = 1 } mbc_bankmode;
typedef enum oam_state_ {
   INACTIVE,
   STARTING,
   ACTIVE,
   RESTARTING,
} oam_state;

oam_state mem_oam_state;
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
void wbyte(word addr, byte val);
void wword(word addr, word val);
void mem_direct_write(word addr, byte val);
byte mem_get_current_rom_bank();
byte mem_direct_read(word addr);
byte rbyte(word addr);
word rword(word addr);

#endif
