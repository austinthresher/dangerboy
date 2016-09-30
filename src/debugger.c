#include "debugger.h"
#include "disas.h"
#include "memory.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

char cmd[256];
byte a, b, c, d, e, h, l;
word sp;
bool ei;
bool need_break;
bool curses_on;
WINDOW *memory_map;

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
      printw("\t  A:  %02X => %02X\n", a, cpu_A);
   }
   if (b != cpu_B) {
      printw("\t  B:  %02X => %02X\n", b, cpu_B);
   }
   if (c != cpu_C) {
      printw("\t  C:  %02X => %02X\n", c, cpu_C);
   }
   if (d != cpu_D) {
      printw("\t  D:  %02X => %02X\n", d, cpu_D);
   }
   if (e != cpu_E) {
      printw("\t  E:  %02X => %02X\n", e, cpu_E);
   }
   if (h != cpu_H) {
      printw("\t  H:  %02X => %02X\n", h, cpu_H);
   }
   if (l != cpu_L) {
      printw("\t  L:  %02X => %02X\n", l, cpu_L);
   }
   if (sp != cpu_SP) {
      printw("\t  SP: %04X => %04X\n", sp, cpu_SP);
   }
   if (ei != cpu_ime) {
      if (ei) {
         printw("\tIME: true  => false\n");
      } else {
         printw("\tIME: false => true\n");
      }
   }
}

void print_memory_map(int x, word addr) {
   if (has_colors()) {
      attron(COLOR_PAIR(2));
   }
   // 2 lines for status bar, 1 for prompt
   int width = COLS - x;
   int height = LINES - 3;
   if (memory_map == NULL) {
      memory_map = newwin(LINES - 3, width, 2, x);
   }
 
   // 4 digits for addr, then a space, and 2 for border
   int char_per_line = width - 7;
   if (char_per_line % 2 == 1) {
      char_per_line--;
   }

   for (int i = 1; i < height-1; ++i) {
      int line_addr = addr + i * (char_per_line / 2);
      wmove(memory_map, i, 1);
      wprintw(memory_map, "%04X ", line_addr);
      for (int b = 0; b < char_per_line / 2; ++b) {
         if (line_addr + b <= 0xFFFF) {
            wprintw(memory_map, "%02X", mem_rb(line_addr + b));
         } else {
            wprintw(memory_map, "..");
         }
      }
   }

   box(memory_map, 0, 0);
   wrefresh(memory_map);

   if(has_colors()) {
      attroff(COLOR_PAIR(2));
   }
}

void print_status_bar() {
   if (has_colors()) {
      attron(COLOR_PAIR(1));
   }

   // Clear the space for the header
   for (int rows = 0; rows < 2; rows++) {
      for(int c = 0; c < COLS; c++) {
         mvprintw(rows, c, " ");
      }
   }
   mvprintw(0, 0, 
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
   mvprintw(1, 0, 
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

   if(has_colors()) {
      attroff(COLOR_PAIR(1));
   }
}

void debugger_cli() {
   char input[256], buff[256];

   if (!curses_on) {
      initscr();
      scrollok(stdscr, true);
      if (has_colors()) {
         start_color();
         init_pair(1, COLOR_BLACK, COLOR_GREEN);
         init_pair(2, COLOR_WHITE, COLOR_BLUE);
      }
      else printw("no colors");
      curses_on = true;
   }

   // Print prompt 
   print_reg_diff();
   disas_at(cpu_PC);

   do {
      print_status_bar();
      refresh();
      print_memory_map(32, 0xFF00);
      mvprintw(LINES - 1, 0, "debug: ");
      refresh();
       getstr(buff);
      if (sc(buff, "")) {
         scrl(-1);
      } else {
         strcpy(cmd, buff);
      }
   } while (!handle_input(cmd));
   
   store_regs();
   refresh();

   if (curses_on && !debugger_should_break()) {
      endwin();
      curses_on = false;
   }
}

bool handle_input(const char* str) {
   if (sc(str, "continue") || sc(str, "c")) {
      strcpy(cmd, "");
      debugger_continue();
      return true;
   }
   if (sc(str, "step") || sc(str, "s")) {
      return true;
   }
   if (sc(str, "quit") || sc(str, "q")) {
      endwin();
      exit(0);
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
