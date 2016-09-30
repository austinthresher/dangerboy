#include "disas.h"
#include <ncurses.h>

#define PRINT(...) \
   printw(__VA_ARGS__); break;

void disas_at(word addr) {
   byte opcode = mem_rb(addr++);
   if (opcode != 0xCB) {
      printw("%02X\t", opcode);
   }
   else {
      printw("%02X %02X\t", opcode, mem_rb(addr++));
   }

   switch (opcode) {
      case 0x00: PRINT("NOP");   
      case 0x01: PRINT("LD BC %04X", mem_rw(addr));
      case 0x02: PRINT("LD A, (BC)");
      case 0x03: PRINT("INC BC");
      case 0x04: PRINT("INC B");
      case 0x05: PRINT("DEC B");
      case 0x06: PRINT("LD B, %02X", mem_rb(addr));
      case 0x07: PRINT("RLCA");
      case 0x08: PRINT("LD %04X, SP", mem_rw(addr));
      case 0x09: PRINT("ADD HL, BC");
      case 0x0A: PRINT("LD A, (BC)");
      case 0x0B: PRINT("DEC BC");
      case 0x0C: PRINT("INC C");
      case 0x0D: PRINT("DEC C");
      case 0x0E: PRINT("LD C, %02X", mem_rb(addr));
      case 0x0F: PRINT("RRCA");

      case 0x10: PRINT("STOP");
      case 0x11: PRINT("LD DE %04X", mem_rw(addr));
      case 0x12: PRINT("LD A, (DE)");
      case 0x13: PRINT("INC DE");
      case 0x14: PRINT("INC D");
      case 0x15: PRINT("DEC D");
      case 0x16: PRINT("LD D, %02X", mem_rb(addr));
      case 0x17: PRINT("RLA");
      case 0x18: PRINT("JR %02X \t; %d", mem_rb(addr), (sbyte)mem_rb(addr));
      case 0x19: PRINT("ADD HL, DE");
      case 0x1A: PRINT("LD A, (DE)");
      case 0x1B: PRINT("DEC DE");
      case 0x1C: PRINT("INC E");
      case 0x1D: PRINT("DEC E");
      case 0x1E: PRINT("LD E, %02X", mem_rb(addr));
      case 0x1F: PRINT("RRA");

      case 0x20: PRINT("JR NZ, %02X \t; %d", mem_rb(addr), (sbyte)mem_rb(addr));
      case 0x21: PRINT("LD HL, %04X", mem_rw(addr));
      case 0x22: PRINT("LD (HL+), A");
      case 0x23: PRINT("INC HL");
      case 0x24: PRINT("INC H");
      case 0x25: PRINT("DEC H");
      case 0x26: PRINT("LD H, %02X", mem_rb(addr));
      case 0x27: PRINT("DAA");
      case 0x28: PRINT("JR Z, %02X \t; %d", mem_rb(addr), (sbyte)mem_rb(addr));
      case 0x29: PRINT("ADD HL, HL");
      case 0x2A: PRINT("LD A, (HL+)");
      case 0x2B: PRINT("DEC HL");
      case 0x2C: PRINT("INC L");
      case 0x2D: PRINT("DEC L");
      case 0x2E: PRINT("LD L, %02X", mem_rb(addr));
      case 0x2F: PRINT("CPL");

      case 0x30: PRINT("JR NC, %02X \t; %d", mem_rb(addr), (sbyte)mem_rb(addr));
      case 0x31: PRINT("LD SP, %04X", mem_rw(addr));
      case 0x32: PRINT("LD (HL-), A");
      case 0x33: PRINT("INC SP");
      case 0x34: PRINT("INC (HL)");
      case 0x35: PRINT("DEC (HL)");
      case 0x36: PRINT("LD (HL), %02X", mem_rb(addr));
      case 0x37: PRINT("SCF");
      case 0x38: PRINT("JR C, %02X \t; %d", mem_rb(addr), (sbyte)mem_rb(addr));
      case 0x39: PRINT("ADD HL, SP");
      case 0x3A: PRINT("LD A, (HL-)");
      case 0x3B: PRINT("DEC SP");
      case 0x3C: PRINT("INC A");
      case 0x3D: PRINT("DEC A");
      case 0x3E: PRINT("LD A, %02X", mem_rb(addr));
      case 0x3F: PRINT("CCF");

      case 0x40: PRINT("LD B, B");
      case 0x41: PRINT("LD B, C");
      case 0x42: PRINT("LD B, D");
      case 0x43: PRINT("LD B, E");
      case 0x44: PRINT("LD B, H");
      case 0x45: PRINT("LD B, L");
      case 0x46: PRINT("LD B, (HL)");
      case 0x47: PRINT("LD B, A");
      case 0x48: PRINT("LD C, B");
      case 0x49: PRINT("LD C, C");
      case 0x4A: PRINT("LD C, D");
      case 0x4B: PRINT("LD C, E");
      case 0x4C: PRINT("LD C, H");
      case 0x4D: PRINT("LD C, L");
      case 0x4E: PRINT("LD C, (HL)");
      case 0x4F: PRINT("LD C, A");

      case 0x50: PRINT("LD D, B");
      case 0x51: PRINT("LD D, C");
      case 0x52: PRINT("LD D, D");
      case 0x53: PRINT("LD D, E");
      case 0x54: PRINT("LD D, H");
      case 0x55: PRINT("LD D, L");
      case 0x56: PRINT("LD D, (HL)");
      case 0x57: PRINT("LD D, A");
      case 0x58: PRINT("LD E, B");
      case 0x59: PRINT("LD E, C");
      case 0x5A: PRINT("LD E, D");
      case 0x5B: PRINT("LD E, E");
      case 0x5C: PRINT("LD E, H");
      case 0x5D: PRINT("LD E, L");
      case 0x5E: PRINT("LD E, (HL)");
      case 0x5F: PRINT("LD E, A");

      case 0x60: PRINT("LD H, B");
      case 0x61: PRINT("LD H, C");
      case 0x62: PRINT("LD H, D");
      case 0x63: PRINT("LD H, E");
      case 0x64: PRINT("LD H, H");
      case 0x65: PRINT("LD H, L");
      case 0x66: PRINT("LD H, (HL)");
      case 0x67: PRINT("LD H, A");
      case 0x68: PRINT("LD L, B");
      case 0x69: PRINT("LD L, C");
      case 0x6A: PRINT("LD L, D");
      case 0x6B: PRINT("LD L, E");
      case 0x6C: PRINT("LD L, H");
      case 0x6D: PRINT("LD L, L");
      case 0x6E: PRINT("LD L, (HL)");
      case 0x6F: PRINT("LD L, A");

      case 0x70: PRINT("LD (HL), B");
      case 0x71: PRINT("LD (HL), C");
      case 0x72: PRINT("LD (HL), D");
      case 0x73: PRINT("LD (HL), E");
      case 0x74: PRINT("LD (HL), H");
      case 0x75: PRINT("LD (HL), L");
      case 0x76: PRINT("HALT");
      case 0x77: PRINT("LD (HL), A");
      case 0x78: PRINT("LD A, B");
      case 0x79: PRINT("LD A, C");
      case 0x7A: PRINT("LD A, D");
      case 0x7B: PRINT("LD A, E");
      case 0x7C: PRINT("LD A, H");
      case 0x7D: PRINT("LD A, L");
      case 0x7E: PRINT("LD A, (HL)");
      case 0x7F: PRINT("LD A, A");

      case 0x80: PRINT("ADD B");
      case 0x81: PRINT("ADD C");
      case 0x82: PRINT("ADD D");
      case 0x83: PRINT("ADD E");
      case 0x84: PRINT("ADD H");
      case 0x85: PRINT("ADD L");
      case 0x86: PRINT("ADD (HL)");
      case 0x87: PRINT("ADD A");
      case 0x88: PRINT("ADC B");
      case 0x89: PRINT("ADC C");
      case 0x8A: PRINT("ADC D");
      case 0x8B: PRINT("ADC E");
      case 0x8C: PRINT("ADC H");
      case 0x8D: PRINT("ADC L");
      case 0x8E: PRINT("ADC (HL)");
      case 0x8F: PRINT("ADC A");

      case 0x90: PRINT("SUB B");
      case 0x91: PRINT("SUB C");
      case 0x92: PRINT("SUB D");
      case 0x93: PRINT("SUB E");
      case 0x94: PRINT("SUB H");
      case 0x95: PRINT("SUB L");
      case 0x96: PRINT("SUB (HL)");
      case 0x97: PRINT("SUB A");
      case 0x98: PRINT("SBC B");
      case 0x99: PRINT("SBC C");
      case 0x9A: PRINT("SBC D");
      case 0x9B: PRINT("SBC E");
      case 0x9C: PRINT("SBC H");
      case 0x9D: PRINT("SBC L");
      case 0x9E: PRINT("SBC (HL)");
      case 0x9F: PRINT("SBC A");

      case 0xA0: PRINT("AND B");
      case 0xA1: PRINT("AND C");
      case 0xA2: PRINT("AND D");
      case 0xA3: PRINT("AND E");
      case 0xA4: PRINT("AND H");
      case 0xA5: PRINT("AND L");
      case 0xA6: PRINT("AND (HL)");
      case 0xA7: PRINT("AND A");
      case 0xA8: PRINT("XOR B");
      case 0xA9: PRINT("XOR C");
      case 0xAA: PRINT("XOR D");
      case 0xAB: PRINT("XOR E");
      case 0xAC: PRINT("XOR H");
      case 0xAD: PRINT("XOR L");
      case 0xAE: PRINT("XOR (HL)");
      case 0xAF: PRINT("XOR A");

      case 0xB0: PRINT("");
      case 0xB1: PRINT("");
      case 0xB2: PRINT("");
      case 0xB3: PRINT("");
      case 0xB4: PRINT("");
      case 0xB5: PRINT("");
      case 0xB6: PRINT("");
      case 0xB7: PRINT("");
      case 0xB8: PRINT("");
      case 0xB9: PRINT("");
      case 0xBA: PRINT("");
      case 0xBB: PRINT("");
      case 0xBC: PRINT("");
      case 0xBD: PRINT("");
      case 0xBE: PRINT("");
      case 0xBF: PRINT("");

default: 
                 break;
   }

   printw("\n");
}
