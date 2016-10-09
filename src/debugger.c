#include "debugger.h"
#include "disas.h"
#include "memory.h"
#include "ppu.h"

#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct BreakpointEntry {
   bool break_on_read;
   bool break_on_write;
   bool break_on_exec;
   bool break_on_equal;
   byte watch_value;
};

word memory_view_addr;
char cmd[256];
byte a, b, c, d, e, h, l;
word sp;
bool ei;
bool need_break;
bool curses_on;
int console_width;
int console_height;
bool show_pc;
bool break_on_op[256];
struct BreakpointEntry breakpoints[0x10000];
WINDOW *memory_map = NULL, *console_pane = NULL, *status_bar = NULL;

bool handle_input(const char* string);
bool sc(const char* strA, const char* strB) { return strcmp(strA, strB) == 0; }

void init_curses() {
   initscr();
   if (has_colors()) {
      start_color();
      init_pair(COL_NORMAL, COLOR_WHITE, COLOR_BLACK);
      init_pair(COL_STATUS, COLOR_BLACK, COLOR_GREEN);
      init_pair(COL_MEMVAL, COLOR_BLUE, COLOR_BLACK);
      init_pair(COL_MEMADD, COLOR_GREEN, COLOR_BLACK);
      init_pair(COL_HILITE, COLOR_YELLOW, COLOR_BLACK); // fix color here
      init_pair(COL_OPCODE, COLOR_RED, COLOR_BLACK);
      init_pair(COL_VALUES, COLOR_BLUE, COLOR_BLACK);
   }
   if (console_pane == NULL) {
      // The 2 here is status bar height. TODO: Change to variable
      console_height = LINES - 2;
      console_width = COLS / 2;
      show_pc = false;
      if (console_width < 32) {
         console_width = 32;
      }
      if (console_width > 40) {
         show_pc = true;
         console_width = 40;
      }
      console_pane = newwin(console_height, console_width, 2, 0);
      scrollok(console_pane, true);
   }
}

void store_regs() {

   // Save the state of CPU registers after each
   // step so that we can view what each operation
   // changed.
   a  = cpu_A;
   b  = cpu_B;
   c  = cpu_C;
   d  = cpu_D;
   e  = cpu_E;
   h  = cpu_H;
   l  = cpu_L;
   sp = cpu_SP;
   ei = cpu_ime;
}

void print_reg_diff() {

   // Print any registers whose values changed
   // since they were last recorded.
   COLOR(console_pane, COL_VALUES);
   if (a != cpu_A) {
      wprintw(console_pane, "A: %02X => %02X\n", a, cpu_A);
   }
   if (b != cpu_B) {
      wprintw(console_pane, "B: %02X => %02X\n", b, cpu_B);
   }
   if (c != cpu_C) {
      wprintw(console_pane, "C: %02X => %02X\n", c, cpu_C);
   }
   if (d != cpu_D) {
      wprintw(console_pane, "D: %02X => %02X\n", d, cpu_D);
   }
   if (e != cpu_E) {
      wprintw(console_pane, "E: %02X => %02X\n", e, cpu_E);
   }
   if (h != cpu_H) {
      wprintw(console_pane, "H: %02X => %02X\n", h, cpu_H);
   }
   if (l != cpu_L) {
      wprintw(console_pane, "L: %02X => %02X\n", l, cpu_L);
   }
   if (sp != cpu_SP) {
      wprintw(console_pane, "SP: %04X => %04X\n", sp, cpu_SP);
   }
   if (ei != cpu_ime) {
      if (ei) {
         wprintw(console_pane, "IME: true => false\n");
      } else {
         wprintw(console_pane, "IME: false => true\n");
      }
   }
   COLOR(console_pane, COL_NORMAL);
}

void print_memory_map(int x, word addr) {
   // 2 lines for status bar, 1 for prompt
   int width = COLS - x;
   int height = console_height - 1;
   if (memory_map == NULL) {
      memory_map = newwin(height, width, 2, x);
   }
 
   // 4 digits for addr, then a space, and 2 for border
   // Right now, this is characters per line
   int bytes_per_line = (width - 7);

   // Display memory in 4 byte + 1 space chunks
   // (4 * 2) + 1 = 9 characters per 4 bytes
   int chunks = bytes_per_line / 9; 
   bytes_per_line = chunks * 5;

   int cur_addr = addr;
   for (int i = 0; i < height-2; ++i) {
      // Clear this row (TODO: There's probably a better way to do this)
      wmove(memory_map, i+1, 1);
      COLOR(memory_map, COL_MEMVAL);
      for (int w = 0; w < width; w++) {
//         wprintw(memory_map, ".");
      }

      // Print the address for this row
      wmove(memory_map, i+1, 1);
      COLOR(memory_map, COL_MEMADD); 
      if (cur_addr <= 0xFFFF) {
         wprintw(memory_map, "%04X ", cur_addr);
      } else {
         wprintw(memory_map, ".... ");
      }

      // Print the values for this memory row
      COLOR(memory_map, COL_MEMVAL);
      for (int b = 0; b < bytes_per_line; ++b) {
         if (b % 5 == 4) {
            wprintw(memory_map, " ");
         } else {
            if (cur_addr <= 0xFFFF) {

               // Check if the value we're printing needs hilighting
               // (breakpoint, program counter, etc)
               bool hilite = false;
               if (cur_addr == cpu_PC) {
                  COLOR(memory_map, COL_OPCODE);
                  hilite = true;
               } else if (breakpoints[cur_addr].break_on_read
                       || breakpoints[cur_addr].break_on_write
                       || breakpoints[cur_addr].break_on_exec
                       || breakpoints[cur_addr].break_on_equal) {
                  COLOR(memory_map, COL_HILITE);
                  hilite = true;
               }
               
               wprintw(memory_map, "%02X", mem_rb(cur_addr++));
              
               // Reset the color if we changed it
               if (hilite) {
                  COLOR(memory_map, COL_MEMVAL);
               }
            } else {
               // Print this if we're out of range
               wprintw(memory_map, "..");
            }
         }
      }
   }
   COLOR(memory_map, COL_NORMAL);
   box(memory_map, 0, 0);
   wrefresh(memory_map);
}

void debugger_log(const char* str) {
   if (curses_on && console_pane != NULL) {
      wprintw(console_pane, "[%ld] ", cpu_ticks);
      wprintw(console_pane, "%s\n", str);
   }
}

// TODO: Make status bar only over memory map,
// different lines for different categories
void print_status_bar() {
   int width = COLS;
   int height = 2;
   if (status_bar == NULL) {
      status_bar = newwin(height, width, 0, 0);
   }
   COLOR(status_bar, COL_STATUS);

   // Clear the space for the header
   box(status_bar, 0, 0);

   wmove(status_bar, 0, 0);
   wprintw(status_bar,
         "[TIMA:%02X TMA:%02X SPD:%d]\t"
         "[DIV:%02X]\t"
         "[IE:%02X IF:%02X]\t"
         "[TICK:%d]",
         mem_rb(TIMA_ADDR),
         mem_rb(TMA_ADDR),
         mem_rb(TIMER_CONTROL_ADDR) & 4,
         mem_rb(DIV_REGISTER_ADDR),
         mem_rb(INT_ENABLED_ADDR),
         mem_rb(INT_FLAG_ADDR),
         cpu_ticks);
   wprintw(status_bar, "\t[IME:%d]", cpu_ime ? 1 : 0);
   wprintw(status_bar, "\t[LCD:%d STAT:%02X LY:%02X LYC:%02X TIMER: %06d]",
         (mem_direct_read(0xFF40) & 0x80) ? 1 : 0,
         mem_direct_read(LCD_STATUS_ADDR),
         mem_direct_read(LCD_LINE_Y_ADDR),
         mem_direct_read(LCD_LINE_Y_C_ADDR),
         ppu_get_timer());
   wprintw(status_bar, "\t[RAM:%d ROM:%02X]",
         mem_current_ram_bank,
         mem_current_rom_bank); 
   if (mem_mbc_type == MBC1) {
      wprintw(status_bar, "\t[MBC1:%d]", mem_mbc_bankmode);
   }
   wmove(status_bar, 1, 0);
   wprintw(status_bar, 
         "[PC:%04X SP:%04X]\t"
         "[A:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X]",
         cpu_PC,
         cpu_SP,
         cpu_A,
         cpu_B,
         cpu_C,
         cpu_D,
         cpu_E,
         cpu_H,
         cpu_L);
   wrefresh(status_bar);
}

void debugger_cli() {
   char buff[256];
   if (!curses_on) {
      init_curses();
      curses_on = true;
   }

   // Print prompt 
   print_reg_diff();
   wmove(console_pane, console_height - 1, 0);
   if (show_pc) {
      COLOR(console_pane, COL_MEMADD);
      wprintw(console_pane, "[%04X] ", cpu_PC);
      COLOR(console_pane, COL_NORMAL);
   }
   disas_at(cpu_PC, console_pane);

   do {
      print_status_bar();
      print_memory_map(console_width, memory_view_addr);

      wmove(console_pane, console_height - 1, 0);
      wprintw(console_pane, "debug: ");
      wrefresh(console_pane);

      // Get input. If input is empty, don't scroll and repeat last command.
      wgetstr(console_pane, buff);
      if (sc(buff, "")) {
         wscrl(console_pane, -1);
      } else {
         strcpy(cmd, buff);
      }
   } while (!handle_input(cmd));
   
   store_regs();
}

void debugger_free() {
   if (curses_on) {
      delwin(console_pane);
      delwin(status_bar);
      delwin(memory_map);
      endwin();
   }
}

bool handle_input(const char* str) {
   
   // Attempt splitting our input at spaces to parse arguments
   char inp[256];
   strcpy(inp, str);
   char *args = strtok(inp, " ");
   if (args == &inp[0]) {
      args = strtok(NULL, " ");
   }
   
   // TODO: Add a reset command

   // Help command
   if (sc(inp, "help") || sc(inp, "h") || sc(inp, "?")) {
      wprintw(console_pane, "Available commands:\n");
      wprintw(console_pane, "\tquit\n\tstep\n\tmem\n\tdisas\n\tbreak\n\tbreakop\n\tcontinue\n");
      return false;
   }

   // Break on opcode
   if (sc(inp, "breakop") || sc(inp, "bo")) {
      if (args == NULL) {
         wprintw(console_pane, "USAGE:\nbreakop [opcode]\n");
         return false;
      }
      int op = (int)strtol(args, NULL, 16);
      if (op < 0 || op > 0xFF) {
         wprintw(console_pane, "Opcode must be from 0 to FF\n");
         return false;
      }
      break_on_op[op] = !break_on_op[op];
      if (break_on_op[op]) {
         wprintw(console_pane, "Breaking on opcode %02X\n", op);
      } else {
         wprintw(console_pane, "Disabled break on opcode %02X\n", op);
      }
      return false;
 
   }

   // Breakpoint command
   if (sc(inp, "break") || sc(inp, "b")) {
      char *addr_str = args;
      char *cmd_str = strtok(NULL, " ");
      char *val_str = strtok(NULL, " ");
      if (addr_str != NULL && cmd_str != NULL) {
         int addr = strtol(addr_str, NULL, 16);
         if (addr < 0 || addr > 0xFFFF) {
            wprintw(console_pane, "invalid address\n");
            return false;
         }
         if (sc(cmd_str, "read") || sc(cmd_str, "r")) {
            debugger_break_on_read(addr);
            wprintw(console_pane, "Breaking on read of %04X\n", addr);
            return false;
         }
         if (sc(cmd_str, "write") || sc(cmd_str, "w")) {
            debugger_break_on_write(addr);
            wprintw(console_pane, "Breaking on write of %04X\n", addr);
            return false;
         }
         if (sc(cmd_str, "exec") || sc(cmd_str, "x")) {
            debugger_break_on_exec(addr);
            wprintw(console_pane, "Breaking on exec of %04X\n", addr);
            return false;
         }
         if (sc(cmd_str, "clear") || sc(cmd_str, "c")) {
            debugger_clear_breakpoint(addr);
            wprintw(console_pane, "Breakpoints at %04X cleared.\n", addr);
            return false;
         }
         if (sc(cmd_str, "equal") || sc(cmd_str, "e")) {
            if (val_str == NULL) {
               wprintw(console_pane, "USAGE:\nbreak [ad] equal [value]\n");
               return false;
            }
            int eq_val = strtol(val_str, NULL, 16);
            if (eq_val < 0 || eq_val > 0xFF) {
               wprintw(console_pane, "invalid value (0 to 0xFF)\n");
               return false;
            }
            debugger_break_on_equal(addr, (byte)eq_val);
            wprintw(console_pane, "Breaking when (%04X) == %02X\n", addr, eq_val);
            return false;
         }
      }
      wprintw(console_pane, "USAGE:\n"
                            "break [addr] [read | r]\n"
                            "break [addr] [write | w]\n"
                            "break [addr] [exec | x]\n"
                            "break [addr] [equal | e] [value]\n"
                            "break [addr] [clear | c]\n");
      return false;
   }

   // Disassemble command
   if (sc(inp, "disas") || sc(inp, "d")) {
      char *start_addr_str = args;
      char *end_addr_str = strtok(NULL, " ");
      if (start_addr_str == NULL || end_addr_str == NULL) {
         wprintw(console_pane, "USAGE:\ndisas [start addr] [end addr]\n");
         return false;
      }
      int start_addr = strtol(start_addr_str, NULL, 16);
      int end_addr = strtol(end_addr_str, NULL, 16);
      if (start_addr > end_addr) {
         wprintw(console_pane, "start addr must be less than end addr\n");
         return false;
      }
      if (start_addr < 0 || start_addr > 0xFFFF
       || end_addr < 0 || end_addr > 0xFFFF) {
         wprintw(console_pane, "invalid range\n");
         return false;
      }
      word add = start_addr;
      while (add <= end_addr) {
         if (show_pc) {
            COLOR(console_pane, COL_MEMADD);
            wprintw(console_pane, "[%04X] ", add);
            COLOR(console_pane, COL_NORMAL);
         }     
         add = disas_at(add, console_pane);
      }
      return false; 
   }

   // Continue execution command
   if (sc(inp, "continue") || sc(inp, "c")) {
      strcpy(cmd, ""); // Clear last command
      debugger_continue();
      return true;
   }

   // Step forward command
   if (sc(inp, "step") || sc(inp, "s")) {
      // TODO: Allow specifying a number of steps to advance
      return true;
   }

   // Quit program command
   if (sc(inp, "quit") || sc(inp, "q")) {
      endwin();
      exit(0);
   }

   // View memory command
   if (sc(inp, "mem") || sc(inp, "m")) {
      if (args == NULL) {
         wprintw(console_pane, "USAGE:\nmem [hex address]\n");
         return false;
      }
      int address = (int)strtol(args, NULL, 16);
      if (address < 0 || address > 0xFFFF) {
         wprintw(console_pane, "Address must be from 0 to FFFF\n");
         return false;
      }
      memory_view_addr = address;
      print_memory_map(32, memory_view_addr);
      return false;
   }

   // Return false = don't advance time, true = advance time
   return false;
}

bool debugger_should_break() { return need_break; }

void debugger_break() { need_break = true; }

void debugger_continue() {
   wprintw(console_pane, "Continuing.\n");
   need_break = false;
}

void debugger_init() {
   store_regs();
   strcpy(cmd, "");
   for (int i = 0; i < 0x10000; ++i) {
      debugger_clear_breakpoint(i);
   }
   for (int i = 0; i < 0x100; ++i) {
      break_on_op[i] = false;
   }
   curses_on = false;
   memory_map = NULL;
   status_bar = NULL;
   console_pane = NULL;
   memory_view_addr = 0;
}


void debugger_clear_breakpoint(word addr) {
   breakpoints[addr].break_on_read  = false;
   breakpoints[addr].break_on_write = false;
   breakpoints[addr].break_on_exec  = false;
   breakpoints[addr].break_on_equal = false;
   breakpoints[addr].watch_value    = 0;
}

void debugger_break_on_read(word addr) {
   breakpoints[addr].break_on_read = true;
}

void debugger_break_on_write(word addr) {
   breakpoints[addr].break_on_write = true;
}

void debugger_break_on_exec(word addr) {
   breakpoints[addr].break_on_exec = true;
}

void debugger_break_on_equal(word addr, byte value) {
   breakpoints[addr].break_on_equal = true;
   breakpoints[addr].watch_value    = value;
}

void debugger_notify_mem_exec(word addr) {
   if (need_break) {
      return;
   }
   if (breakpoints[addr].break_on_exec) {
      need_break = true;
      memory_view_addr = addr;

      wprintw(console_pane, "[%ld] ", cpu_ticks);
      wprintw(console_pane, "%04X is about to execute. Breaking.\n", addr);
   } else if (break_on_op[mem_rb(addr)]) {
      need_break = true;
      memory_view_addr = addr;
      wprintw(console_pane, "[%ld] ", cpu_ticks);
      wprintw(console_pane, "%02X is about to execute. Breaking.\n", mem_rb(addr));
   }
}

void debugger_notify_mem_write(word addr, byte val) {
   if (need_break) {
      return;
   }
   if (breakpoints[addr].break_on_equal
         && breakpoints[addr].watch_value == val) {
      need_break = true;
      memory_view_addr = addr;
      wprintw(console_pane, "[%ld] ", cpu_ticks);
      wprintw(console_pane, "%02X was written to %04X. Breaking.\n", val, addr);
   }
   else if (breakpoints[addr].break_on_write) {
      need_break = true;
      memory_view_addr = addr;
      wprintw(console_pane, "[%ld] ", cpu_ticks);
      wprintw(console_pane, "%04X was written to. Breaking.\n", addr);
   }
}

void debugger_notify_mem_read(word addr) {
   if (need_break) {
      return;
   }
   if (breakpoints[addr].break_on_read) {
      need_break = true;
      memory_view_addr = addr;
      wprintw(console_pane, "[%ld] ", cpu_ticks);
      wprintw(console_pane, "%04X was read. Breaking.\n", addr);
   }
}
