#ifndef __LCD_H__
#define __LCD_H__

#include "cpu.h"
#include "datatypes.h"

// External defines
#define LCD_MODE_HBLANK 0 // Lasts ~200 cycles
#define LCD_MODE_VBLANK 1 // Lasts ~4560 cycles
#define LCD_MODE_OAM 2    // Lasts ~84 cycles
#define LCD_MODE_VRAM 3   // Lasts ~172 cycles
#define LCD_WHITE 0xFE
#define LCD_LITE 0xC6
#define LCD_DARK 0x7C
#define LCD_BLACK 0x00

byte lcd_vram[160 * 144 * 3];
bool lcd_draw;

void lcd_init();
void lcd_reset();
void lcd_advance_time(tick ticks);
byte lcd_reg_read(word addr);
void lcd_reg_write(word addr, byte val);
tick lcd_get_timer();
bool lcd_disabled();
#endif
