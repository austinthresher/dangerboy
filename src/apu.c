#include "apu.h"
#include "memory.h"

// ----------------
// Internal defines
// ----------------

#define BUFFER_SIZE 512
#define TYPE int16_t

// ------------------
// Internal variables
// ------------------

TYPE buffer_left[BUFFER_SIZE];
TYPE buffer_right[BUFFER_SIZE];

byte apu_reg_read(word addr) { return dread(addr); }

void apu_reg_write(word addr, byte val) { dwrite(addr, val); }

void apu_advance_time(cycle cycles) {}
