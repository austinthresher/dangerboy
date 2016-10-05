#ifndef __CPU_H__
#define __CPU_H__

#include "datatypes.h"
#include "memory.h"

#define BITMASK_C 0x10
#define BITMASK_H 0x20
#define BITMASK_N 0x40
#define BITMASK_Z 0x80
#define INT_VBLANK 0x01
#define INT_STAT 0x02
#define INT_TIMA 0x04
#define INT_SERIAL 0x08
#define INT_INPUT 0x10

// For debugging
byte cpu_last_op;
word cpu_last_pc;

// Registers
byte cpu_A;
byte cpu_B;
byte cpu_C;
byte cpu_D;
byte cpu_E;
byte cpu_H;
byte cpu_L;
word cpu_PC; // Program Counter
word cpu_SP; // Stack Pointer
bool cpu_ime_delay;
bool cpu_ime;
bool cpu_halted;
bool cpu_stopped;
tick cpu_ticks;
void (*cpu_opcodes[0x100])();

void cpu_execute_step();
void cpu_init();
void cpu_reset();
void cpu_advance_time(tick dt);
void cpu_reset_timer();

#endif
