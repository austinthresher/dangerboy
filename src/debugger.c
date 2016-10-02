#include "debugger.h"
#include "disas.h"
#include "memory.h"

#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

word memory_view_addr;
char cmd[256];
byte a, b, c, d, e, h, l;
word sp;
bool ei;
bool need_break;
bool curses_on;
// TODO: Finish switching main display to windows
WINDOW *memory_map, *debug_pane, *status_bar;

struct BreakpointEntry {
   bool break_on_read;
   bool break_on_write;
   bool break_on_exec;
   bool break_on_equal;
   byte watch_value;
};

struct BreakpointEntry* breakpoints;

bool handle_input(const char* string);
bool sc(const char* strA, const char* strB) { return strcmp(strA, strB) == 0; }

void store_regs() {
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
   if (a != cpu_A) {
      wprintw(debug_pane, "\t  A:  %02X => %02X\n", a, cpu_A);
   }
   if (b != cpu_B) {
      wprintw(debug_pane, "\t  B:  %02X => %02X\n", b, cpu_B);
   }
   if (c != cpu_C) {
      wprintw(debug_pane, "\t  C:  %02X => %02X\n", c, cpu_C);
   }
   if (d != cpu_D) {
      wprintw(debug_pane, "\t  D:  %02X => %02X\n", d, cpu_D);
   }
   if (e != cpu_E) {
      wprintw(debug_pane, "\t  E:  %02X => %02X\n", e, cpu_E);
   }
   if (h != cpu_H) {
      wprintw(debug_pane, "\t  H:  %02X => %02X\n", h, cpu_H);
   }
   if (l != cpu_L) {
      wprintw(debug_pane, "\t  L:  %02X => %02X\n", l, cpu_L);
   }
   if (sp != cpu_SP) {
      wprintw(debug_pane, "\t  SP: %04X => %04X\n", sp, cpu_SP);
   }
   if (ei != cpu_ime) {
      if (ei) {
         wprintw(debug_pane, "\tIME: true  => false\n");
      } else {
         wprintw(debug_pane, "\tIME: false => true\n");
      }
   }
}


void print_memory_map(int x, word addr) {
   // 2 lines for status bar, 1 for prompt
   int width = COLS - x;
   int height = LINES - 3;
   if (memory_map == NULL) {
      memory_map = newwin(height, width, 2, x);
   }
 
   // 4 digits for addr, then a space, and 2 for border
   int bytes_per_line = (width - 6) / 2;
   // Display memory in 4 byte chunks
   int chunks = bytes_per_line / 5; 
   bytes_per_line = chunks * 5;;

   for (int i = 0; i < height-2; ++i) {
      int line_addr = addr + i * bytes_per_line;
      wmove(memory_map, i+1, 1);
      if (has_colors()) {
         wattron(memory_map, COLOR_PAIR(3));
      }
      if (line_addr <= 0xFFFF) {
         wprintw(memory_map, "%04X ", line_addr);
      } else {
         wprintw(memory_map, ".... ");
      }
      if (has_colors()) {
         wattron(memory_map, COLOR_PAIR(2));
      }
      for (int b = 0; b < bytes_per_line; ++b) {
         if (line_addr + b <= 0xFFFF) {
            if (b % 5 == 4) {
               wprintw(memory_map, " ");
            } else {
               wprintw(memory_map, "%02X", mem_rb(line_addr + b));
            }
         } else {
            wprintw(memory_map, "..");
         }
      }

   }
   if(has_colors()) {
      wattroff(memory_map, COLOR_PAIR(3));
      wattroff(memory_map, COLOR_PAIR(2));
   }
   box(memory_map, 0, 0);
   wrefresh(memory_map);
}

void print_status_bar() {
   int width = COLS;
   int height = 2;
   if (status_bar == NULL) {
      status_bar = newwin(height, width, 0, 0);
   }   
   if (has_colors()) {
      wattron(status_bar, COLOR_PAIR(1));
   }

   // Clear the space for the header
   for (int rows = 0; rows < 2; rows++) {
      for(int c = 0; c < COLS; c++) {
         wmove(status_bar, rows, c);
         wprintw(status_bar, ".");
      }
   }
   wmove(status_bar, 0, 0);
   wprintw(status_bar,
         "[TIMA:%02X TMA:%02X SPD:%d]\t"
         "[DIV:%02X]\t"
         "[IE:%02X IF:%02X]\t"
         "[TICK:%d]",
         mem_rb(TIMA_ADDR),
         mem_rb(TMA_ADDR),
         mem_rb(TIMER_CONTROL_ADDR) & 0x3,
         mem_rb(DIV_REGISTER_ADDR),
         mem_rb(INT_ENABLED_ADDR),
         mem_rb(INT_FLAG_ADDR),
         cpu_ticks);
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
   char input[256], buff[256];

   if (!curses_on) {
      initscr();
      if (has_colors()) {
         start_color();
         init_pair(1, COLOR_BLACK, COLOR_GREEN);
         init_pair(2, COLOR_BLACK, COLOR_BLUE);
         init_pair(3, COLOR_WHITE, COLOR_BLUE);
      }
      if (debug_pane == NULL) {
         debug_pane = newwin(LINES - 2, 32, 2, 0);
         scrollok(debug_pane, true);
         wsetscrreg(debug_pane, 0, LINES - 3);
         idlok(debug_pane, true);
       }
      curses_on = true;
      print_memory_map(32, memory_view_addr);
   }

   // Print prompt 
   print_reg_diff();
   wmove(debug_pane, LINES - 3, 0);
   disas_at(cpu_PC, debug_pane);

   do {
      print_status_bar();

      print_memory_map(32, memory_view_addr);
      wmove(debug_pane, LINES - 3, 0);
      wprintw(debug_pane, "debug: ");
      wrefresh(debug_pane);
//      wmove(debug_pane, LINES - 3, 0);
      wgetstr(debug_pane, buff);
      if (sc(buff, "")) {
        wscrl(debug_pane, -1);
      } else {
         strcpy(cmd, buff);
      }
   } while (!handle_input(cmd));
   
   store_regs();
   //refresh();

   if (curses_on && !debugger_should_break()) {
      endwin();
      curses_on = false;
   }
}

bool handle_input(const char* str) {
   char inp[256];
   strcpy(inp, str);
   char *args = strtok(inp, " ");
   if (args == &inp[0]) {
      args = strtok(NULL, " ");
   }
   if (sc(inp, "help") || sc(inp, "h") || sc(inp, "?")) {
      wprintw(debug_pane, "Available commands:\n");
      wprintw(debug_pane, "\tquit\n\tstep\n\tmem\n\tcontinue\n");
   }
   if (sc(inp, "continue") || sc(inp, "c")) {
      strcpy(cmd, ""); // Clear last command
      debugger_continue();
      return true;
   }
   if (sc(inp, "step") || sc(inp, "s")) {
      return true;
   }
   if (sc(inp, "quit") || sc(inp, "q")) {
      endwin();
      exit(0);
   }
   if (sc(inp, "mem") || sc(inp, "m")) {
      if (args == NULL) {
         wprintw(debug_pane, "USAGE: mem [hex address]\n");
         return false;
      }
      int address = (int)strtol(args, NULL, 16);
      if (address < 0 || address > 0xFFFF) {
         wprintw(debug_pane, "Address must be from 0 to FFFF\n");
         return false;
      }
      memory_view_addr = address;
      print_memory_map(32, memory_view_addr);
      return false;
   }
   return false;
}

bool debugger_should_break() { return need_break; }

void debugger_break() { need_break = true; }

void debugger_continue() {
   printf("Continuing.\n");
   need_break = false;
}

void debugger_init() {
   store_regs();
   strcpy(cmd, "");
   breakpoints = calloc(0x10000, sizeof(struct BreakpointEntry));
   for (int i = 0; i < 0x10000; ++i) {
      debugger_clear_breakpoint(i);
   }
   curses_on = false;
   memory_map = NULL;
   memory_view_addr = 0;
}

void debugger_free() {
   if (breakpoints != NULL) {
      free(breakpoints);
   }
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
   if (breakpoints[addr].break_on_exec) {
      need_break = true;
   }
}

void debugger_notify_mem_write(word addr, byte val) {
   if (breakpoints[addr].break_on_write) {
      need_break = true;
   }
   if (breakpoints[addr].break_on_equal
         && breakpoints[addr].watch_value == val) {
      need_break = true;
   }
}

void debugger_notify_mem_read(word addr) {
   if (breakpoints[addr].break_on_read) {
      need_break = true;
   }
}
