#ifndef __APU_H__
#define __APU_H__

#include "defines.h"

byte apu_reg_read(word addr);
void apu_reg_write(word addr, byte val);
void apu_advance_time(cycle cycles);

#endif
