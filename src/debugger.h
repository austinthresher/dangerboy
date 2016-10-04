#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "datatypes.h"

// ncurses utility defines
// TODO: Move shared ncurses stuff to its own .h
#define COL_NORMAL 0
#define COL_STATUS 1
#define COL_MEMVAL 2
#define COL_MEMADD 3
#define COL_HILITE 4
#define COL_OPCODE 5
#define COL_VALUES 6

#define COLOR(w, x) if(has_colors()) wattrset((w), COLOR_PAIR(x));

bool debugger_should_break();
void debugger_cli();
void debugger_break();
void debugger_continue();
void debugger_init();
void debugger_free();
void debugger_clear_breakpoint(word addr);
void debugger_break_on_read(word addr);
void debugger_break_on_write(word addr);
void debugger_break_on_exec(word addr);
void debugger_break_on_equal(word addr, byte value);
void debugger_notify_mem_exec(word addr);
void debugger_notify_mem_write(word addr, byte val);
void debugger_notify_mem_read(word addr);

#endif
