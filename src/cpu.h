#ifndef __CPU_H__
#define __CPU_H__

#include "defines.h"
#include "memory.h"

typedef struct cpu_state_ {
   word pc;                  // Program Counter
   word sp;                  // Stack Pointer
   byte a, b, c, d, e, h, l; // Registers
   bool hf, cf, zf, nf;      // Flags
   bool halted;
   bool stopped;
   bool ime;
   bool ime_delay;
} cpu_state;

// This is used to track time in the debugger
cycle cpu_ticks;

cpu_state cpu_get_state();
void cpu_execute_step();
void cpu_init();
void cpu_reset();
void cpu_advance_time(cycle dt);
void cpu_reset_timer();
void cpu_serial_transfer();
bool cpu_serial_active();
// For debugging
byte get_last_op();
word get_last_pc();

#endif
