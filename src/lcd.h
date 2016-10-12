#ifndef __LCD_H__
#define __LCD_H__

#include "cpu.h"
#include "defines.h"

void lcd_reset();
void lcd_advance_time(cycle cycles);
byte lcd_reg_read(word addr);
void lcd_reg_write(word addr, byte val);
cycle lcd_get_timer();
bool lcd_disabled();
bool lcd_vram_accessible();
bool lcd_oam_accessible();
bool lcd_ready();
byte* lcd_get_framebuffer();
#endif
