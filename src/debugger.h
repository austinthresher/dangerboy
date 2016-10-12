#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "defines.h"

// ncurses utility defines
// TODO: Move shared ncurses stuff to its own .h
#define COL_NORMAL 0
#define COL_STATUS 1
#define COL_MEMVAL 2
#define COL_MEMADD 3
#define COL_HILITE 4
#define COL_OPCODE 5
#define COL_VALUES 6

#define COLOR(w, x)  \
   if (has_colors()) \
      wattrset((w), COLOR_PAIR(x));

bool dbg_should_break();
void dbg_cli();
void dbg_break();
void dbg_continue();
void dbg_init();
void dbg_free();
void dbg_log(const char* str);
void dbg_clear_breakpoint(word addr);
void dbg_break_on_read(word addr);
void dbg_break_on_write(word addr);
void dbg_break_on_exec(word addr);
void dbg_break_on_equal(word addr, byte value);
void dbg_notify_exec(word addr);
void dbg_notify_write(word addr, byte val);
void dbg_notify_read(word addr);

#endif
