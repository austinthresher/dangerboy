#include "datatypes.h"

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

