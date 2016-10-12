#ifndef __DATATYPES_H__
#define __DATATYPES_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// -----
// Types
// -----

#define byte uint8_t
#define word uint16_t
#define cycle int64_t
#define sbyte int8_t

// ----------------
// Memory Locations
// ----------------

#define ROMNAME 0x0134
#define CARTTYPE 0x0147
#define ROMSIZE 0x0148
#define RAMSIZE 0x0149
#define OAMSTART 0xFE00
#define OAMEND 0xFEA0
#define JOYP 0xFF00
#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07
#define IF 0xFF0F
#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY 0xFF42
#define SCX 0xFF43
#define LY 0xFF44
#define LYC 0xFF45
#define DMA 0xFF46
#define BGPAL 0xFF47
#define OBJPAL 0xFF48
#define WINX 0xFF4B
#define WINY 0xFF4A
#define IE 0xFFFF
#define CH1SWEEP 0xFF10
#define CH1LENGTH 0xFF11
#define CH1VOLUME 0xFF12
#define CH1LOFREQ 0xFF13
#define CH1HIFREQ 0xFF14
#define CH2LENGTH 0xFF16
#define CH2VOLUME 0xFF17
#define CH2LOFREQ 0xFF18
#define CH2HIFREQ 0xFF19
#define CH3ENABLE 0xFF1A
#define CH3LENGTH 0xFF1B
#define CH3VOLUME 0xFF1C
#define CH3LODATA 0xFF1D
#define CH3HIDATA 0xFF1E
#define CH4LENGTH 0xFF20
#define CH4VOLUME 0xFF21
#define CH4POLY   0xFF22
#define CH4CONSEC 0xFF23
#define WAVETABLE 0xFF30

// ----------
// Interrupts
// ----------

#define INT_VBLANK 0x01
#define INT_STAT 0x02
#define INT_TIMA 0x04
#define INT_SERIAL 0x08
#define INT_INPUT 0x10


#endif
