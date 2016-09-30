#include "debugger.h"
#include "disas.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

bool need_break;
char cmd[256];

byte a, b, c, d, e, h, l;
word sp;
bool ei;
bool curses_on;

struct BreakpointEntry { 
   bool break_on_read;
   bool break_on_write;
   bool break_on_exec;
   bool break_on_equal;
   byte watch_value;
};

struct BreakpointEntry *breakpoints;

bool handle_input(const char *string);
bool sc(const char *strA, const char *strB) {
   return strcmp(strA, strB) == 0;
}

void store_regs() {
   a  = cpu_A;
   b  = cpu_B;
   c  = cpu_C;
   d  = cpu_D;
   e  = cpu_E;
   h  = cpu_H;
   l  = cpu_L;
   sp = cpu_SP;
   ei = cpu_ei;
}

void print_reg_diff() {
   if (a != cpu_A)   { printw("\t  A:  %02X => %02X\n", a, cpu_A); }
   if (b != cpu_B)   { printw("\t  B:  %02X => %02X\n", b, cpu_B); }
   if (c != cpu_C)   { printw("\t  C:  %02X => %02X\n", c, cpu_C); }
   if (d != cpu_D)   { printw("\t  D:  %02X => %02X\n", d, cpu_D); }
   if (e != cpu_E)   { printw("\t  E:  %02X => %02X\n", e, cpu_E); }
   if (h != cpu_H)   { printw("\t  H:  %02X => %02X\n", h, cpu_H); }
   if (l != cpu_L)   { printw("\t  L:  %02X => %02X\n", l, cpu_L); }
   if (sp != cpu_SP) { printw("\t  SP: %04X => %04X\n", sp, cpu_SP); }
   if (ei != cpu_ei) {
      if (ei) { printw("\tEI: true  => false\n"); }
      else    { printw("\tEI: false => true\n"); }
   }
}

void debugger_cli() {
   
   if (!curses_on) {
      initscr();
      scrollok(stdscr, true);
      curses_on = true;
   }

   char input[256];
   char buff[256];
   print_reg_diff();
   mvprintw(LINES-1, 0, "[%04X] ", cpu_PC);
   disas_at(cpu_PC);
   refresh();
   do {
      printw("debug: "); 
      refresh();
      getstr(buff);
      if (sc(buff, "")) {
         scrl(-1);
      } else {
         strcpy(cmd, buff);
      }
   } while (!handle_input(cmd));
   store_regs();
//   scrl(1);
   refresh();
   if (curses_on && !debugger_should_break()) {
      endwin();
      curses_on = false;
   }
}

bool handle_input(const char *str) {
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

bool debugger_should_break() {
   return need_break;
}

void debugger_break() {
   need_break = true;
}

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
   breakpoints[addr].watch_value = value;
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
