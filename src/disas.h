#ifndef __DISAS_H__
#define __DISAS_H__

#include "cpu.h"
#include "defines.h"
#include "memory.h"

#include <ncurses.h>

word disas_at(word addr, WINDOW* win);

#endif
