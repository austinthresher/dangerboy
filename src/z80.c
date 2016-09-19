#include "z80.h"

#define GET_FLAG_C FLAG_C
#define GET_FLAG_H FLAG_H
#define GET_FLAG_Z FLAG_Z
#define GET_FLAG_N FLAG_N

#define SET_FLAG_C(v) FLAG_C = (v)
#define SET_FLAG_H(v) FLAG_H = (v)
#define SET_FLAG_N(v) FLAG_N = (v)
#define SET_FLAG_Z(v) FLAG_Z = (v)

static bool FLAG_C;
static bool FLAG_H;
static bool FLAG_Z;
static bool FLAG_N;

void z80_init(char* romname) {
   z80_interrupts_enabled       = false;
   z80_delayed_enable_interrupt = 0;
   z80_tima_timer               = 0;
   z80_div_timer                = 0;
   z80_halt                     = false;
   z80_stop                     = false;
   z80_rom_fname                = romname;

   for (size_t i = 0; i < 0x100; i++) {
      z80_opcodes[i] = &z80_NI;
   }

   z80_opcodes[0x00] = &z80_NOP;
   z80_opcodes[0x01] = &z80_LDBC_nn;
   z80_opcodes[0x02] = &z80_LD_AT_BC_A;
   z80_opcodes[0x03] = &z80_INC16_BC;
   z80_opcodes[0x04] = &z80_INC_B;
   z80_opcodes[0x05] = &z80_DEC_B;
   z80_opcodes[0x06] = &z80_LDB_n;
   z80_opcodes[0x07] = &z80_RLCA;
   z80_opcodes[0x08] = &z80_LD_nn_SP;
   z80_opcodes[0x09] = &z80_ADD16_HL_BC;
   z80_opcodes[0x0A] = &z80_LDA_AT_BC;
   z80_opcodes[0x0B] = &z80_DEC16_BC;
   z80_opcodes[0x0C] = &z80_INC_C;
   z80_opcodes[0x0D] = &z80_DEC_C;
   z80_opcodes[0x0E] = &z80_LDC_n;
   z80_opcodes[0x0F] = &z80_RRCA;

   z80_opcodes[0x10] = &z80_STOP;
   z80_opcodes[0x11] = &z80_LDDE_nn;
   z80_opcodes[0x12] = &z80_LD_AT_DE_A;
   z80_opcodes[0x13] = &z80_INC16_DE;
   z80_opcodes[0x14] = &z80_INC_D;
   z80_opcodes[0x15] = &z80_DEC_D;
   z80_opcodes[0x16] = &z80_LDD_n;
   z80_opcodes[0x17] = &z80_RLA;
   z80_opcodes[0x18] = &z80_JR_n;
   z80_opcodes[0x19] = &z80_ADD16_HL_DE;
   z80_opcodes[0x1A] = &z80_LDA_AT_DE;
   z80_opcodes[0x1B] = &z80_DEC16_DE;
   z80_opcodes[0x1C] = &z80_INC_E;
   z80_opcodes[0x1D] = &z80_DEC_E;
   z80_opcodes[0x1E] = &z80_LDE_n;
   z80_opcodes[0x1F] = &z80_RRA;

   z80_opcodes[0x20] = &z80_JR_NZ_n;
   z80_opcodes[0x21] = &z80_LDHL_nn;
   z80_opcodes[0x22] = &z80_LD_AT_HLI_A;
   z80_opcodes[0x23] = &z80_INC16_HL;
   z80_opcodes[0x24] = &z80_INC_H;
   z80_opcodes[0x25] = &z80_DEC_H;
   z80_opcodes[0x26] = &z80_LDH_n;
   z80_opcodes[0x27] = &z80_DAA;
   z80_opcodes[0x28] = &z80_JR_Z_n;
   z80_opcodes[0x29] = &z80_ADD16_HL_HL;
   z80_opcodes[0x2A] = &z80_LDA_AT_HLI;
   z80_opcodes[0x2B] = &z80_DEC16_HL;
   z80_opcodes[0x2C] = &z80_INC_L;
   z80_opcodes[0x2D] = &z80_DEC_L;
   z80_opcodes[0x2E] = &z80_LDL_n;
   z80_opcodes[0x2F] = &z80_CPL;

   z80_opcodes[0x30] = &z80_JR_NC_n;
   z80_opcodes[0x31] = &z80_LDSP_nn;
   z80_opcodes[0x32] = &z80_LD_AT_HLD_A;
   z80_opcodes[0x33] = &z80_INC16_SP;
   z80_opcodes[0x34] = &z80_INC_AT_HL;
   z80_opcodes[0x35] = &z80_DEC_AT_HL;
   z80_opcodes[0x36] = &z80_LD_AT_HL_n;
   z80_opcodes[0x37] = &z80_SCF;
   z80_opcodes[0x38] = &z80_JR_C_n;
   z80_opcodes[0x39] = &z80_ADD16_HL_SP;
   z80_opcodes[0x3A] = &z80_LDA_AT_HLD;
   z80_opcodes[0x3B] = &z80_DEC16_SP;
   z80_opcodes[0x3C] = &z80_INC_A;
   z80_opcodes[0x3D] = &z80_DEC_A;
   z80_opcodes[0x3E] = &z80_LDA_n;
   z80_opcodes[0x3F] = &z80_CCF;

   z80_opcodes[0x40] = &z80_LDB_B;
   z80_opcodes[0x41] = &z80_LDB_C;
   z80_opcodes[0x42] = &z80_LDB_D;
   z80_opcodes[0x43] = &z80_LDB_E;
   z80_opcodes[0x44] = &z80_LDB_H;
   z80_opcodes[0x45] = &z80_LDB_L;
   z80_opcodes[0x46] = &z80_LDB_AT_HL;
   z80_opcodes[0x47] = &z80_LDB_A;
   z80_opcodes[0x48] = &z80_LDC_B;
   z80_opcodes[0x49] = &z80_LDC_C;
   z80_opcodes[0x4A] = &z80_LDC_D;
   z80_opcodes[0x4B] = &z80_LDC_E;
   z80_opcodes[0x4C] = &z80_LDC_H;
   z80_opcodes[0x4D] = &z80_LDC_L;
   z80_opcodes[0x4E] = &z80_LDC_AT_HL;
   z80_opcodes[0x4F] = &z80_LDC_A;

   z80_opcodes[0x50] = &z80_LDD_B;
   z80_opcodes[0x51] = &z80_LDD_C;
   z80_opcodes[0x52] = &z80_LDD_D;
   z80_opcodes[0x53] = &z80_LDD_E;
   z80_opcodes[0x54] = &z80_LDD_H;
   z80_opcodes[0x55] = &z80_LDD_L;
   z80_opcodes[0x56] = &z80_LDD_AT_HL;
   z80_opcodes[0x57] = &z80_LDD_A;
   z80_opcodes[0x58] = &z80_LDE_B;
   z80_opcodes[0x59] = &z80_LDE_C;
   z80_opcodes[0x5A] = &z80_LDE_D;
   z80_opcodes[0x5B] = &z80_LDE_E;
   z80_opcodes[0x5C] = &z80_LDE_H;
   z80_opcodes[0x5D] = &z80_LDE_L;
   z80_opcodes[0x5E] = &z80_LDE_AT_HL;
   z80_opcodes[0x5F] = &z80_LDE_A;

   z80_opcodes[0x60] = &z80_LDH_B;
   z80_opcodes[0x61] = &z80_LDH_C;
   z80_opcodes[0x62] = &z80_LDH_D;
   z80_opcodes[0x63] = &z80_LDH_E;
   z80_opcodes[0x64] = &z80_LDH_H;
   z80_opcodes[0x65] = &z80_LDH_L;
   z80_opcodes[0x66] = &z80_LDH_AT_HL;
   z80_opcodes[0x67] = &z80_LDH_A;
   z80_opcodes[0x68] = &z80_LDL_B;
   z80_opcodes[0x69] = &z80_LDL_C;
   z80_opcodes[0x6A] = &z80_LDL_D;
   z80_opcodes[0x6B] = &z80_LDL_E;
   z80_opcodes[0x6C] = &z80_LDL_H;
   z80_opcodes[0x6D] = &z80_LDL_L;
   z80_opcodes[0x6E] = &z80_LDL_AT_HL;
   z80_opcodes[0x6F] = &z80_LDL_A;

   z80_opcodes[0x70] = &z80_LD_AT_HL_B;
   z80_opcodes[0x71] = &z80_LD_AT_HL_C;
   z80_opcodes[0x72] = &z80_LD_AT_HL_D;
   z80_opcodes[0x73] = &z80_LD_AT_HL_E;
   z80_opcodes[0x74] = &z80_LD_AT_HL_H;
   z80_opcodes[0x75] = &z80_LD_AT_HL_L;
   z80_opcodes[0x76] = &z80_HALT;
   z80_opcodes[0x77] = &z80_LD_AT_HL_A;
   z80_opcodes[0x78] = &z80_LDA_B;
   z80_opcodes[0x79] = &z80_LDA_C;
   z80_opcodes[0x7A] = &z80_LDA_D;
   z80_opcodes[0x7B] = &z80_LDA_E;
   z80_opcodes[0x7C] = &z80_LDA_H;
   z80_opcodes[0x7D] = &z80_LDA_L;
   z80_opcodes[0x7E] = &z80_LDA_AT_HL;
   z80_opcodes[0x7F] = &z80_LDA_A;

   z80_opcodes[0x80] = &z80_ADD_A_B;
   z80_opcodes[0x81] = &z80_ADD_A_C;
   z80_opcodes[0x82] = &z80_ADD_A_D;
   z80_opcodes[0x83] = &z80_ADD_A_E;
   z80_opcodes[0x84] = &z80_ADD_A_H;
   z80_opcodes[0x85] = &z80_ADD_A_L;
   z80_opcodes[0x86] = &z80_ADD_A_AT_HL;
   z80_opcodes[0x87] = &z80_ADD_A_A;
   z80_opcodes[0x88] = &z80_ADC_A_B;
   z80_opcodes[0x89] = &z80_ADC_A_C;
   z80_opcodes[0x8A] = &z80_ADC_A_D;
   z80_opcodes[0x8B] = &z80_ADC_A_E;
   z80_opcodes[0x8C] = &z80_ADC_A_H;
   z80_opcodes[0x8D] = &z80_ADC_A_L;
   z80_opcodes[0x8E] = &z80_ADC_A_AT_HL;
   z80_opcodes[0x8F] = &z80_ADC_A_A;

   z80_opcodes[0x90] = &z80_SUB_A_B;
   z80_opcodes[0x91] = &z80_SUB_A_C;
   z80_opcodes[0x92] = &z80_SUB_A_D;
   z80_opcodes[0x93] = &z80_SUB_A_E;
   z80_opcodes[0x94] = &z80_SUB_A_H;
   z80_opcodes[0x95] = &z80_SUB_A_L;
   z80_opcodes[0x96] = &z80_SUB_A_AT_HL;
   z80_opcodes[0x97] = &z80_SUB_A_A;
   z80_opcodes[0x98] = &z80_SBC_A_B;
   z80_opcodes[0x99] = &z80_SBC_A_C;
   z80_opcodes[0x9A] = &z80_SBC_A_D;
   z80_opcodes[0x9B] = &z80_SBC_A_E;
   z80_opcodes[0x9C] = &z80_SBC_A_H;
   z80_opcodes[0x9D] = &z80_SBC_A_L;
   z80_opcodes[0x9E] = &z80_SBC_A_AT_HL;
   z80_opcodes[0x9F] = &z80_SBC_A_A;

   z80_opcodes[0xA0] = &z80_AND_B;
   z80_opcodes[0xA1] = &z80_AND_C;
   z80_opcodes[0xA2] = &z80_AND_D;
   z80_opcodes[0xA3] = &z80_AND_E;
   z80_opcodes[0xA4] = &z80_AND_H;
   z80_opcodes[0xA5] = &z80_AND_L;
   z80_opcodes[0xA6] = &z80_AND_AT_HL;
   z80_opcodes[0xA7] = &z80_AND_A;
   z80_opcodes[0xA8] = &z80_XOR_B;
   z80_opcodes[0xA9] = &z80_XOR_C;
   z80_opcodes[0xAA] = &z80_XOR_D;
   z80_opcodes[0xAB] = &z80_XOR_E;
   z80_opcodes[0xAC] = &z80_XOR_H;
   z80_opcodes[0xAD] = &z80_XOR_L;
   z80_opcodes[0xAE] = &z80_XOR_AT_HL;
   z80_opcodes[0xAF] = &z80_XOR_A;

   z80_opcodes[0xB0] = &z80_OR_B;
   z80_opcodes[0xB1] = &z80_OR_C;
   z80_opcodes[0xB2] = &z80_OR_D;
   z80_opcodes[0xB3] = &z80_OR_E;
   z80_opcodes[0xB4] = &z80_OR_H;
   z80_opcodes[0xB5] = &z80_OR_L;
   z80_opcodes[0xB6] = &z80_OR_AT_HL;
   z80_opcodes[0xB7] = &z80_OR_A;
   z80_opcodes[0xB8] = &z80_CP_B;
   z80_opcodes[0xB9] = &z80_CP_C;
   z80_opcodes[0xBA] = &z80_CP_D;
   z80_opcodes[0xBB] = &z80_CP_E;
   z80_opcodes[0xBC] = &z80_CP_H;
   z80_opcodes[0xBD] = &z80_CP_L;
   z80_opcodes[0xBE] = &z80_CP_AT_HL;
   z80_opcodes[0xBF] = &z80_CP_A;

   z80_opcodes[0xC0] = &z80_RET_NZ;
   z80_opcodes[0xC1] = &z80_POPBC;
   z80_opcodes[0xC2] = &z80_JP_NZ_nn;
   z80_opcodes[0xC3] = &z80_JP_nn;
   z80_opcodes[0xC4] = &z80_CALL_NZ_nn;
   z80_opcodes[0xC5] = &z80_PUSHBC;
   z80_opcodes[0xC6] = &z80_ADD_A_n;
   z80_opcodes[0xC7] = &z80_RST_00H;
   z80_opcodes[0xC8] = &z80_RET_Z;
   z80_opcodes[0xC9] = &z80_RET;
   z80_opcodes[0xCA] = &z80_JP_Z_nn;
   z80_opcodes[0xCB] = &z80_CB;
   z80_opcodes[0xCC] = &z80_CALL_Z_nn;
   z80_opcodes[0xCD] = &z80_CALL_nn;
   z80_opcodes[0xCE] = &z80_ADC_A_n;
   z80_opcodes[0xCF] = &z80_RST_08H;

   z80_opcodes[0xD0] = &z80_RET_NC;
   z80_opcodes[0xD1] = &z80_POPDE;
   z80_opcodes[0xD2] = &z80_JP_NC_nn;
   z80_opcodes[0xD3] = &z80_NI;
   z80_opcodes[0xD4] = &z80_CALL_NC_nn;
   z80_opcodes[0xD5] = &z80_PUSHDE;
   z80_opcodes[0xD6] = &z80_SUB_A_n;
   z80_opcodes[0xD7] = &z80_RST_10H;
   z80_opcodes[0xD8] = &z80_RET_C;
   z80_opcodes[0xD9] = &z80_RETI;
   z80_opcodes[0xDA] = &z80_JP_C_nn;
   z80_opcodes[0xDB] = &z80_NI;
   z80_opcodes[0xDC] = &z80_CALL_C_nn;
   z80_opcodes[0xDD] = &z80_NI;
   z80_opcodes[0xDE] = &z80_SBC_A_n;
   z80_opcodes[0xDF] = &z80_RST_18H;

   z80_opcodes[0xE0] = &z80_LD_n_A;
   z80_opcodes[0xE1] = &z80_POPHL;
   z80_opcodes[0xE2] = &z80_LD_AT_C_A;
   z80_opcodes[0xE3] = &z80_NI;
   z80_opcodes[0xE4] = &z80_NI;
   z80_opcodes[0xE5] = &z80_PUSHHL;
   z80_opcodes[0xE6] = &z80_AND_n;
   z80_opcodes[0xE7] = &z80_RST_20H;
   z80_opcodes[0xE8] = &z80_ADD16_SP_n;
   z80_opcodes[0xE9] = &z80_JP_AT_HL;
   z80_opcodes[0xEA] = &z80_LD_AT_nn_A;
   z80_opcodes[0xEB] = &z80_NI;
   z80_opcodes[0xEC] = &z80_NI;
   z80_opcodes[0xED] = &z80_NI;
   z80_opcodes[0xEE] = &z80_XOR_n;
   z80_opcodes[0xEF] = &z80_RST_28H;

   z80_opcodes[0xF0] = &z80_LD_A_n;
   z80_opcodes[0xF1] = &z80_POPAF;
   z80_opcodes[0xF2] = &z80_LDA_AT_C;
   z80_opcodes[0xF3] = &z80_DI;
   z80_opcodes[0xF4] = &z80_NI;
   z80_opcodes[0xF5] = &z80_PUSHAF;
   z80_opcodes[0xF6] = &z80_OR_n;
   z80_opcodes[0xF7] = &z80_RST_30H;
   z80_opcodes[0xF8] = &z80_LDHL_SP_n;
   z80_opcodes[0xF9] = &z80_LDSP_HL;
   z80_opcodes[0xFA] = &z80_LDA_AT_nn;
   z80_opcodes[0xFB] = &z80_EI;
   z80_opcodes[0xFC] = &z80_NI;
   z80_opcodes[0xFD] = &z80_NI;
   z80_opcodes[0xFE] = &z80_CP_A_n;
   z80_opcodes[0xFF] = &z80_RST_38H;

   z80_reset();
}

void z80_reset() {
   z80_PC    = 0x0100;
   z80_A     = 0x01;
   z80_B     = 0x00;
   z80_C     = 0x13;
   z80_D     = 0x00;
   z80_E     = 0xD8;
   z80_SP    = 0xFFFE;
   z80_H     = 0x01;
   z80_L     = 0x4D;
   z80_ticks = 0;
   z80_dt    = 0;

   FLAG_C = true;
   FLAG_H = true;
   FLAG_Z = true;
   FLAG_N = false;


   mem_init();

   // Setup our in-memory registers
   mem_wb(INPUT_REGISTER_ADDR, 0xFF);
   mem_wb(DIV_REGISTER_ADDR, 0xAF);
   mem_wb(TIMER_CONTROL_ADDR, 0xF8);
   mem_wb(0xFF10, 0x80);
   mem_wb(0xFF11, 0xBF);
   mem_wb(0xFF12, 0xF3);
   mem_wb(0xFF14, 0xBF);
   mem_wb(0xFF16, 0x3F);
   mem_wb(0xFF19, 0xBF);
   mem_wb(0xFF1A, 0x7F);
   mem_wb(0xFF1B, 0xFF);
   mem_wb(0xFF1C, 0x9F);
   mem_wb(0xFF1E, 0xBF);
   mem_wb(0xFF20, 0xFF);
   mem_wb(0xFF23, 0xBF);
   mem_wb(0xFF24, 0x77);
   mem_wb(0xFF25, 0xF3);
   mem_wb(0xFF26, 0xF1);
   mem_wb(LCD_CONTROL_ADDR, 0x91);
   mem_wb(0xFF47, 0xFC);
   mem_wb(0xFF48, 0xFF);
   mem_wb(0xFF49, 0xFF);

   mem_load_image(z80_rom_fname);
   mem_get_rom_info();
}

// Flag stuff
void z80_clear_flags() {
   FLAG_Z = FLAG_C = FLAG_H = FLAG_N = false;
}

tick z80_execute_step() {

   // Check interrupts
   bool interrupted = false;
   byte int_IE = mem_rb(INT_ENABLED_ADDR);
   byte int_IF = mem_rb(INT_FLAG_ADDR);
   byte irq = int_IE & int_IF;

   if (int_IF != 0) {
      z80_stop = false;
      z80_halt = false;
   }

   if (z80_interrupts_enabled) {
      byte target = 0x00;
      if ((irq & INT_VBLANK) != 0) {
         target = 0x40;
         mem_wb(INT_FLAG_ADDR, int_IF & ~INT_VBLANK);
         DEBUG("VBLANK INTERRUPT\n");
      } else if ((irq & INT_STAT) != 0) {
         target = 0x48;
         mem_wb(INT_FLAG_ADDR, int_IF & ~INT_STAT);
         DEBUG("STAT INTERRUPT\n");
      } else if ((irq & INT_TIMA) != 0) {
         target = 0x50;
         mem_wb(INT_FLAG_ADDR, int_IF & ~INT_TIMA);
         DEBUG("TIMA INTERRUPT\n");
      } else if ((irq & INT_INPUT) != 0) {
         target = 0x60;
         mem_wb(INT_FLAG_ADDR, int_IF & ~INT_INPUT);
         DEBUG("INPUT INTERRUPT\n");
      }
      if (target != 0x00) {
         interrupted = true;
         z80_interrupts_enabled = false;
         z80_SP -= 2;
         mem_ww(z80_SP, z80_PC);
         z80_PC = target;
         z80_dt = 20;
      }
   }

   if (!interrupted) {
      if (!z80_halt && !z80_stop) {
         z80_dt      = 1;
         z80_last_op = mem_rb(z80_PC);
         z80_last_pc = z80_PC;

         (*z80_opcodes[mem_rb(z80_PC++)])();

         if (z80_delayed_enable_interrupt > 0) {
            z80_delayed_enable_interrupt--;
            if (z80_delayed_enable_interrupt == 0) {
               DEBUG("INTERRUPTS ENABLED\n");
               z80_interrupts_enabled = true;
            }
         }
      } else {
         z80_dt = 4;
      }
   }
   z80_ticks += z80_dt;
   z80_div_timer += z80_dt;

   if (z80_div_timer >= 0xFF) {
      z80_div_timer = 0;
      mem_direct_write(DIV_REGISTER_ADDR,
            (mem_rb(DIV_REGISTER_ADDR) + 1));
   }

   // Bit 3 enables or disables timers
   if (mem_rb(TIMER_CONTROL_ADDR) & 0x04) {
      z80_tima_timer += z80_dt;
      bool inc = false;
      // Bits 1-2 control the timer speed
      switch (mem_rb(TIMER_CONTROL_ADDR) & 0x3) {
         case 0: inc = z80_tima_timer >= 1024; break;
         case 1: inc = z80_tima_timer >= 16; break;
         case 2: inc = z80_tima_timer >= 64; break;
         case 3: inc = z80_tima_timer >= 256; break;
         default: ERROR("timer error");
      }
      if (inc) { 
         z80_tima_timer = 0;
         mem_wb(TIMA_REGISTER_ADDR, mem_rb(TIMA_REGISTER_ADDR) + 1);
         if (mem_rb(TIMA_REGISTER_ADDR) == 0) {
            mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_TIMA);
            mem_wb(TIMA_REGISTER_ADDR, mem_rb(TIMA_MODULO_ADDR));
         }
      }
   }

   return z80_dt;
}

void z80_NI() {
   ERROR("Invalid instruction at PC %04X, potentially opcode %02X ",
         z80_PC, mem_rb(z80_PC - 1));
}

void z80_NOP() {
   z80_dt = 4;
}

// LOAD / STORES

void z80_STORE(byte hi, byte lo, byte data) {
   mem_wb((hi << 8) | lo, data);
   z80_dt = 8;
}

byte z80_FETCH(byte hi, byte lo) {
   z80_dt = 8;
   return mem_rb((hi << 8) | lo);
}

void z80_LOAD(byte* reg, byte data) {
   *reg   = data;
   z80_dt = 4;
}

void z80_LDA_n() {
   z80_LOAD(&z80_A, mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_LDB_n() {
   z80_LOAD(&z80_B, mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_LDC_n() {
   z80_LOAD(&z80_C, mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_LDD_n() {
   z80_LOAD(&z80_D, mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_LDE_n() {
   z80_LOAD(&z80_E, mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_LDH_n() {
   z80_LOAD(&z80_H, mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_LDL_n() {
   z80_LOAD(&z80_L, mem_rb(z80_PC++));
   z80_dt = 8;
}

// LD r1, r2
void z80_LDA_A() {
   z80_LOAD(&z80_A, z80_A);
}

void z80_LDA_B() {
   z80_LOAD(&z80_A, z80_B);
}

void z80_LDA_C() {
   z80_LOAD(&z80_A, z80_C);
}

void z80_LDA_D() {
   z80_LOAD(&z80_A, z80_D);
}

void z80_LDA_E() {
   z80_LOAD(&z80_A, z80_E);
}

void z80_LDA_H() {
   z80_LOAD(&z80_A, z80_H);
}

void z80_LDA_L() {
   z80_LOAD(&z80_A, z80_L);
}

void z80_LDA_AT_HL() {
   z80_LOAD(&z80_A, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LDB_A() {
   z80_LOAD(&z80_B, z80_A);
}

void z80_LDB_B() {
   z80_LOAD(&z80_B, z80_B);
}

void z80_LDB_C() {
   z80_LOAD(&z80_B, z80_C);
}

void z80_LDB_D() {
   z80_LOAD(&z80_B, z80_D);
}

void z80_LDB_E() {
   z80_LOAD(&z80_B, z80_E);
}

void z80_LDB_H() {
   z80_LOAD(&z80_B, z80_H);
}

void z80_LDB_L() {
   z80_LOAD(&z80_B, z80_L);
}

void z80_LDB_AT_HL() {
   z80_LOAD(&z80_B, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LDC_A() {
   z80_LOAD(&z80_C, z80_A);
}

void z80_LDC_B() {
   z80_LOAD(&z80_C, z80_B);
}

void z80_LDC_C() {
   z80_LOAD(&z80_C, z80_C);
}

void z80_LDC_D() {
   z80_LOAD(&z80_C, z80_D);
}

void z80_LDC_E() {
   z80_LOAD(&z80_C, z80_E);
}

void z80_LDC_H() {
   z80_LOAD(&z80_C, z80_H);
}

void z80_LDC_L() {
   z80_LOAD(&z80_C, z80_L);
}

void z80_LDC_AT_HL() {
   z80_LOAD(&z80_C, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LDD_A() {
   z80_LOAD(&z80_D, z80_A);
}

void z80_LDD_B() {
   z80_LOAD(&z80_D, z80_B);
}

void z80_LDD_C() {
   z80_LOAD(&z80_D, z80_C);
}

void z80_LDD_D() {
   z80_LOAD(&z80_D, z80_D);
}

void z80_LDD_E() {
   z80_LOAD(&z80_D, z80_E);
}

void z80_LDD_H() {
   z80_LOAD(&z80_D, z80_H);
}

void z80_LDD_L() {
   z80_LOAD(&z80_D, z80_L);
}

void z80_LDD_AT_HL() {
   z80_LOAD(&z80_D, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LDE_A() {
   z80_LOAD(&z80_E, z80_A);
}

void z80_LDE_B() {
   z80_LOAD(&z80_E, z80_B);
}

void z80_LDE_C() {
   z80_LOAD(&z80_E, z80_C);
}

void z80_LDE_D() {
   z80_LOAD(&z80_E, z80_D);
}

void z80_LDE_E() {
   z80_LOAD(&z80_E, z80_E);
}

void z80_LDE_H() {
   z80_LOAD(&z80_E, z80_H);
}

void z80_LDE_L() {
   z80_LOAD(&z80_E, z80_L);
}

void z80_LDE_AT_HL() {
   z80_LOAD(&z80_E, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LDH_A() {
   z80_LOAD(&z80_H, z80_A);
}

void z80_LDH_B() {
   z80_LOAD(&z80_H, z80_B);
}

void z80_LDH_C() {
   z80_LOAD(&z80_H, z80_C);
}

void z80_LDH_D() {
   z80_LOAD(&z80_H, z80_D);
}

void z80_LDH_E() {
   z80_LOAD(&z80_H, z80_E);
}

void z80_LDH_H() {
   z80_LOAD(&z80_H, z80_H);
}

void z80_LDH_L() {
   z80_LOAD(&z80_H, z80_L);
}

void z80_LDH_AT_HL() {
   z80_LOAD(&z80_H, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LDL_A() {
   z80_LOAD(&z80_L, z80_A);
}

void z80_LDL_B() {
   z80_LOAD(&z80_L, z80_B);
}

void z80_LDL_C() {
   z80_LOAD(&z80_L, z80_C);
}

void z80_LDL_D() {
   z80_LOAD(&z80_L, z80_D);
}

void z80_LDL_E() {
   z80_LOAD(&z80_L, z80_E);
}

void z80_LDL_H() {
   z80_LOAD(&z80_L, z80_H);
}

void z80_LDL_L() {
   z80_LOAD(&z80_L, z80_L);
}

void z80_LDL_AT_HL() {
   z80_LOAD(&z80_L, z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_LD_AT_HL_B() {
   z80_STORE(z80_H, z80_L, z80_B);
}

void z80_LD_AT_HL_C() {
   z80_STORE(z80_H, z80_L, z80_C);
}

void z80_LD_AT_HL_D() {
   z80_STORE(z80_H, z80_L, z80_D);
}

void z80_LD_AT_HL_E() {
   z80_STORE(z80_H, z80_L, z80_E);
}

void z80_LD_AT_HL_H() {
   z80_STORE(z80_H, z80_L, z80_H);
}

void z80_LD_AT_HL_L() {
   z80_STORE(z80_H, z80_L, z80_L);
}

void z80_LD_AT_HL_n() {
   z80_STORE(z80_H, z80_L, mem_rb(z80_PC++));
   z80_dt = 12;
}

void z80_LDA_AT_BC() {
   z80_LOAD(&z80_A, z80_FETCH(z80_B, z80_C));
   z80_dt = 8;
}

void z80_LDA_AT_DE() {
   z80_LOAD(&z80_A, z80_FETCH(z80_D, z80_E));
   z80_dt = 8;
}

void z80_LDA_AT_nn() {
   z80_LOAD(&z80_A, mem_rb(mem_rw(z80_PC)));
   z80_PC += 2;
   z80_dt = 16;
}

// 0x02
void z80_LD_AT_BC_A() {
   z80_STORE(z80_B, z80_C, z80_A);
}

void z80_LD_AT_DE_A() {
   z80_STORE(z80_D, z80_E, z80_A);
}

void z80_LD_AT_HL_A() {
   z80_STORE(z80_H, z80_L, z80_A);
}

void z80_LD_AT_nn_A() {
   mem_wb(mem_rw(z80_PC), z80_A);
   z80_PC += 2;
   z80_dt = 16;
}

void z80_LDA_AT_C() {
   z80_LOAD(&z80_A, mem_rb(0xFF00 + z80_C));
   z80_dt = 8;
}

void z80_LD_AT_C_A() {
   z80_STORE(0xFF, z80_C, z80_A);
}

void z80_LDA_AT_HLD() {
   z80_A = z80_FETCH(z80_H, z80_L);
   z80_DEC16_HL();
}

void z80_LDA_AT_HLI() {
   z80_A = z80_FETCH(z80_H, z80_L);
   z80_INC16_HL();
}

void z80_LD_AT_HLD_A() {
   z80_STORE(z80_H, z80_L, z80_A);
   z80_DEC16_HL();
}

void z80_LD_AT_HLI_A() {
   z80_STORE(z80_H, z80_L, z80_A);
   z80_INC16_HL();
}

void z80_LD_n_A() {
   z80_STORE(0xFF, mem_rb(z80_PC++), z80_A);
   z80_dt = 12;
}

void z80_LD_A_n() {
   z80_A  = z80_FETCH(0xFF, mem_rb(z80_PC++));
   z80_dt = 12;
}

// 0x01
void z80_LDBC_nn() {
   z80_LOAD(&z80_C, mem_rb(z80_PC++));
   z80_LOAD(&z80_B, mem_rb(z80_PC++));
   z80_dt = 12;
}

void z80_LDDE_nn() {
   z80_LOAD(&z80_E, mem_rb(z80_PC++));
   z80_LOAD(&z80_D, mem_rb(z80_PC++));
   z80_dt = 12;
}

void z80_LDHL_nn() {
   z80_LOAD(&z80_L, mem_rb(z80_PC++));
   z80_LOAD(&z80_H, mem_rb(z80_PC++));
   z80_dt = 12;
}

void z80_LDSP_nn() {
   z80_SP = mem_rw(z80_PC);
   z80_PC += 2;
   z80_dt = 12;
}

void z80_LDSP_HL() {
   z80_SP = (z80_H << 8) | z80_L;
   z80_dt = 8;
}

void z80_LDHL_SP_n() {
   byte next = mem_rb(z80_PC++);
   sbyte off = (sbyte)next;
   int res  = off + z80_SP;
   z80_clear_flags();
   SET_FLAG_C((z80_SP & 0xFF) + next > 0xFF);
   SET_FLAG_H((z80_SP & 0xF) + (next & 0xF) > 0xF);
   z80_H  = (res & 0xFF00) >> 8;
   z80_L  = res & 0x00FF;
   z80_dt = 12;
}

void z80_LD_nn_SP() {
   word temp_result = mem_rw(z80_PC);
   z80_PC += 2;
   mem_ww(temp_result, z80_SP);
   z80_dt = 20;
}

// PUSH / POP

void z80_PUSH(byte hi, byte low) {
   mem_wb(--z80_SP, hi);
   mem_wb(--z80_SP, low);
   z80_dt = 16;
}

void z80_POP(byte* hi, byte* low) {
   (*low) = mem_rb(z80_SP++);
   (*hi)  = mem_rb(z80_SP++);
   z80_dt = 12;
}

// PUSH

void z80_PUSHAF() {
   byte flags = 0;
   if (FLAG_Z) {
      flags |= BITMASK_Z;
   }
   if (FLAG_C) {
      flags |= BITMASK_C;
   }
   if (FLAG_H) {
      flags |= BITMASK_H;
   }
   if (FLAG_N) {
      flags |= BITMASK_N;
   }
   z80_PUSH(z80_A, flags);
}

void z80_PUSHBC() {
   z80_PUSH(z80_B, z80_C);
}

void z80_PUSHDE() {
   z80_PUSH(z80_D, z80_E);
}

void z80_PUSHHL() {
   z80_PUSH(z80_H, z80_L);
}

// POP

void z80_POPAF() {
   byte flags = 0;
   z80_POP(&z80_A, &flags);
   FLAG_Z = (flags & BITMASK_Z);
   FLAG_C = (flags & BITMASK_C);
   FLAG_H = (flags & BITMASK_H);
   FLAG_N = (flags & BITMASK_N);
}

void z80_POPBC() {
   z80_POP(&z80_B, &z80_C);
}

void z80_POPDE() {
   z80_POP(&z80_D, &z80_E);
}

void z80_POPHL() {
   z80_POP(&z80_H, &z80_L);
}

// AND / OR / XOR

void z80_AND(byte inp) {
   z80_A = z80_A & inp;
   z80_clear_flags();
   SET_FLAG_H(true);
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_OR(byte inp) {
   z80_A = z80_A | inp;
   z80_clear_flags();
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_XOR(byte inp) {
   z80_A = z80_A ^ inp;
   z80_clear_flags();
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_AND_A() {
   z80_AND(z80_A);
}

void z80_AND_B() {
   z80_AND(z80_B);
}

void z80_AND_C() {
   z80_AND(z80_C);
}

void z80_AND_D() {
   z80_AND(z80_D);
}

void z80_AND_E() {
   z80_AND(z80_E);
}

void z80_AND_H() {
   z80_AND(z80_H);
}

void z80_AND_L() {
   z80_AND(z80_L);
}

void z80_AND_AT_HL() {
   z80_AND(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_AND_n() {
   z80_AND(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_OR_A() {
   z80_OR(z80_A);
}

void z80_OR_B() {
   z80_OR(z80_B);
}

void z80_OR_C() {
   z80_OR(z80_C);
}

void z80_OR_D() {
   z80_OR(z80_D);
}

void z80_OR_E() {
   z80_OR(z80_E);
}

void z80_OR_H() {
   z80_OR(z80_H);
}

void z80_OR_L() {
   z80_OR(z80_L);
}

void z80_OR_AT_HL() {
   z80_OR(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_OR_n() {
   z80_OR(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_XOR_A() {
   z80_XOR(z80_A);
}

void z80_XOR_B() {
   z80_XOR(z80_B);
}

void z80_XOR_C() {
   z80_XOR(z80_C);
}

void z80_XOR_D() {
   z80_XOR(z80_D);
}

void z80_XOR_E() {
   z80_XOR(z80_E);
}

void z80_XOR_H() {
   z80_XOR(z80_H);
}

void z80_XOR_L() {
   z80_XOR(z80_L);
}

void z80_XOR_AT_HL() {
   z80_XOR(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_XOR_n() {
   z80_XOR(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_RLC(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   SET_FLAG_C(t & 0x80);
   t = (t << 1) | GET_FLAG_C;
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_RRC(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   SET_FLAG_C(t & 0x01);
   t = (t >> 1) | (GET_FLAG_C << 7);
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_RL(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   byte old_carry = GET_FLAG_C;
   SET_FLAG_C(t & 0x80);
   t = (t << 1) | old_carry;
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_RR(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   byte old_carry = GET_FLAG_C;
   SET_FLAG_C(t & 0x01);
   t = (t >> 1) | (old_carry << 7);
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_SLA(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   SET_FLAG_C(t & 0x80);
   t = t << 1;
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_SRA(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }
   
   byte msb = t & 0x80;
   SET_FLAG_C(t & 0x01);
   t = (t >> 1) | msb;
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_SWAP(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   t = ((t << 4) | (t >> 4));
   z80_clear_flags();
   SET_FLAG_Z(t == 0);

   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_SRL(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   SET_FLAG_C(t & 0x01);
   t = t >> 1;
   SET_FLAG_Z(t == 0);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 8;

   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_BIT(byte* inp, byte bit) {
   byte t;
   z80_dt = 8;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
      z80_dt += 4;
   }

   SET_FLAG_Z(!(t & (1 << bit)));
   SET_FLAG_N(false);
   SET_FLAG_H(true);
}

void z80_RES(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   t = t & ~(1 << bit);

   z80_dt = 8;
   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_SET(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = z80_FETCH(z80_H, z80_L);
   }

   t = t | (1 << bit);

   z80_dt = 8;
   if (inp == NULL) {
      z80_STORE(z80_H, z80_L, t);
      z80_dt = 16;
   } else {
      *inp = t;
   }
}

void z80_CB() {
   byte  sub_op = mem_rb(z80_PC++);
   byte* regs[] = {&z80_B, &z80_C, &z80_D, &z80_E,
                   &z80_H, &z80_L, NULL,   &z80_A};
   byte index = sub_op & 0x0F;
   if (index >= 8) {
      index -= 8;
      switch (sub_op & 0xF0) {
         case 0x00: z80_RRC(regs[index]); break;
         case 0x10: z80_RR(regs[index]); break;
         case 0x20: z80_SRA(regs[index]); break;
         case 0x30: z80_SRL(regs[index]); break;
         case 0x40: z80_BIT(regs[index], 1); break;
         case 0x50: z80_BIT(regs[index], 3); break;
         case 0x60: z80_BIT(regs[index], 5); break;
         case 0x70: z80_BIT(regs[index], 7); break;
         case 0x80: z80_RES(regs[index], 1); break;
         case 0x90: z80_RES(regs[index], 3); break;
         case 0xA0: z80_RES(regs[index], 5); break;
         case 0xB0: z80_RES(regs[index], 7); break;
         case 0xC0: z80_SET(regs[index], 1); break;
         case 0xD0: z80_SET(regs[index], 3); break;
         case 0xE0: z80_SET(regs[index], 5); break;
         case 0xF0: z80_SET(regs[index], 7); break;
         default: ERROR("CB opcode error"); break;
      }

   } else {
      switch (sub_op & 0xF0) {
         case 0x00: z80_RLC(regs[index]); break;
         case 0x10: z80_RL(regs[index]); break;
         case 0x20: z80_SLA(regs[index]); break;
         case 0x30: z80_SWAP(regs[index]); break;
         case 0x40: z80_BIT(regs[index], 0); break;
         case 0x50: z80_BIT(regs[index], 2); break;
         case 0x60: z80_BIT(regs[index], 4); break;
         case 0x70: z80_BIT(regs[index], 6); break;
         case 0x80: z80_RES(regs[index], 0); break;
         case 0x90: z80_RES(regs[index], 2); break;
         case 0xA0: z80_RES(regs[index], 4); break;
         case 0xB0: z80_RES(regs[index], 6); break;
         case 0xC0: z80_SET(regs[index], 0); break;
         case 0xD0: z80_SET(regs[index], 2); break;
         case 0xE0: z80_SET(regs[index], 4); break;
         case 0xF0: z80_SET(regs[index], 6); break;
         default: ERROR("CB opcode error"); break;
      }
   }
}

// JUMP / RETURN

void z80_JP_nn() {
   z80_PC = mem_rw(z80_PC);
   z80_dt = 16;
}

void z80_JP_NZ_nn() {
   z80_dt = 12;
   if (!GET_FLAG_Z) {
      z80_JP_nn();
   } else {
      z80_PC += 2;
   }
}

void z80_JP_Z_nn() {
   z80_dt = 12;
   if (GET_FLAG_Z) {
      z80_JP_nn();
   } else {
      z80_PC += 2;
   }
}

void z80_JP_NC_nn() {
   z80_dt = 12;
   if (!GET_FLAG_C) {
      z80_JP_nn();
   } else {
      z80_PC += 2;
   }
}

void z80_JP_C_nn() {
   z80_dt = 12;
   if (GET_FLAG_C) {
      z80_JP_nn();
   } else {
      z80_PC += 2;
   }
}

void z80_JP_AT_HL() {
   z80_PC = ((z80_H << 8) | z80_L);
   z80_dt = 4;
}

void z80_JR_n() {
   sbyte offset = mem_rb(z80_PC++);
   z80_PC += offset;
   z80_dt = 12;
}

void z80_JR_NZ_n() {
   z80_dt = 8;
   if (!GET_FLAG_Z) {
      z80_JR_n();
   } else {
      z80_PC++;
   }
}

void z80_JR_Z_n() {
   z80_dt = 8;
   if (GET_FLAG_Z) {
      z80_JR_n();
   } else {
      z80_PC++;
   }
}

void z80_JR_NC_n() {
   z80_dt = 8;
   if (!GET_FLAG_C) {
      z80_JR_n();
   } else {
      z80_PC++;
   }
}

void z80_JR_C_n() {
   z80_dt = 8;
   if (GET_FLAG_C) {
      z80_JR_n();
   } else {
      z80_PC++;
   }
}

void z80_CALL() {
   z80_PUSH((z80_PC + 2) >> 8, (z80_PC + 2) & 0x00FF);
   z80_PC = mem_rw(z80_PC);
   z80_dt = 24;
}

void z80_CALL_nn() {
   z80_CALL();
}

void z80_CALL_NZ_nn() {
   z80_dt = 12;
   if (!GET_FLAG_Z) {
      z80_CALL();
   } else {
      z80_PC += 2;
   }
}

void z80_CALL_Z_nn() {
   z80_dt = 12;
   if (GET_FLAG_Z) {
      z80_CALL();
   } else {
      z80_PC += 2;
   }
}

void z80_CALL_NC_nn() {
   z80_dt = 12;
   if (!GET_FLAG_C) {
      z80_CALL();
   } else {
      z80_PC += 2;
   }
}

void z80_CALL_C_nn() {
   z80_dt = 12;
   if (GET_FLAG_C) {
      z80_CALL();
   } else {
      z80_PC += 2;
   }
}

// Restarts

void z80_RST_00H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x00;
   z80_dt   = 16;
}

void z80_RST_08H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x08;
   z80_dt   = 16;
}

void z80_RST_10H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x10;
   z80_dt   = 16;
}

void z80_RST_18H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x18;
   z80_dt   = 16;
}

void z80_RST_20H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x20;
   z80_dt   = 16;
}

void z80_RST_28H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x28;
   z80_dt   = 16;
}

void z80_RST_30H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x30;
   z80_dt   = 16;
}

void z80_RST_38H() {
   z80_PUSH(z80_PC >> 8, z80_PC & 0x00FF);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x38;
   z80_dt   = 16;

   // If reset 38 is pointing to itself, error out
   if (mem_rb(z80_PC) == 0xFF) {
      ERROR("Reset 38H loop detected.\n");
   }
}

// Returns

void z80_RETURN() {
   z80_PC = mem_rw(z80_SP);
   z80_SP += 2;
   z80_dt = 20;
}

void z80_RET() {
   z80_RETURN();
   z80_dt = 16;
}

void z80_RET_NZ() {
   z80_dt = 8;
   if (!GET_FLAG_Z) {
      z80_RETURN();
   }
}

void z80_RET_Z() {
   z80_dt = 8;
   if (GET_FLAG_Z) {
      z80_RETURN();
   }
}

void z80_RET_NC() {
   z80_dt = 8;
   if (!GET_FLAG_C) {
      z80_RETURN();
   }
}

void z80_RET_C() {
   z80_dt = 8;
   if (GET_FLAG_C) {
      z80_RETURN();
   }
}

void z80_RETI() {
   z80_interrupts_enabled = true;
   z80_RETURN();
   z80_dt = 16;
}

// 16 bit math

void z80_ADD16(byte* a_hi, byte* a_low, byte b_hi, byte b_low) {
   word a = (*a_hi << 8) | *a_low;
   word b = (b_hi << 8) | b_low;

   uint32_t carryCheck = a + b;
   SET_FLAG_H((0x0FFF & a) + (0x0FFF & b) > 0x0FFF);
   SET_FLAG_C(carryCheck > 0x0000FFFF);
   SET_FLAG_N(false);

   a += b;
   *a_hi  = (a & 0xFF00) >> 8;
   *a_low = a & 0x00FF;
   z80_dt = 8;
}

void z80_INC16(byte* hi, byte* low) {
   if ((*low) == 0xFF) {
      (*hi)++;
   }
   (*low)++;

   z80_dt = 8;
}

void z80_DEC16(byte* hi, byte* low) {
   if ((*low) == 0) {
      (*hi)--;
   }
   (*low)--;

   z80_dt = 8;
}

void z80_ADD16_HL_BC() {
   z80_ADD16(&z80_H, &z80_L, z80_B, z80_C);
}

void z80_ADD16_HL_DE() {
   z80_ADD16(&z80_H, &z80_L, z80_D, z80_E);
}

void z80_ADD16_HL_HL() {
   z80_ADD16(&z80_H, &z80_L, z80_H, z80_L);
}

void z80_ADD16_HL_SP() {
   z80_ADD16(&z80_H, &z80_L, (z80_SP & 0xFF00) >> 8, z80_SP & 0x00FF);
}

void z80_ADD16_SP_n() {
   byte  val = mem_rb(z80_PC++);
   sbyte off = (sbyte)val;
   z80_clear_flags();
   SET_FLAG_H((z80_SP & 0xF) + (val & 0xF) > 0xF);
   SET_FLAG_C(((z80_SP & 0xFF) + val > 0xFF));
   z80_SP += off;
   z80_dt = 16;
}

// 0x03
void z80_INC16_BC() {
   z80_INC16(&z80_B, &z80_C);
}

void z80_INC16_DE() {
   z80_INC16(&z80_D, &z80_E);
}

void z80_INC16_HL() {
   z80_INC16(&z80_H, &z80_L);
}

void z80_INC16_SP() {
   z80_SP++;
   z80_dt = 8;
}

void z80_DEC16_BC() {
   z80_DEC16(&z80_B, &z80_C);
}

void z80_DEC16_DE() {
   z80_DEC16(&z80_D, &z80_E);
}

void z80_DEC16_HL() {
   z80_DEC16(&z80_H, &z80_L);
}

void z80_DEC16_SP() {
   z80_SP--;
   z80_dt = 8;
}

// Rotates / misc

void z80_RLA() {
   z80_RL(&z80_A);
   SET_FLAG_Z(false);
   z80_dt = 4;
}

void z80_RLCA() {
   z80_RLC(&z80_A);
   SET_FLAG_Z(false);
   z80_dt = 4;
}

void z80_RRCA() {
   z80_RRC(&z80_A);
   SET_FLAG_Z(false);
   z80_dt = 4;
}

void z80_RRA() {
   z80_RR(&z80_A);
   SET_FLAG_Z(false);
   z80_dt = 4;
}

void z80_DI() {
   z80_interrupts_enabled = false;
   z80_dt                 = 4;
   DEBUG("INTERRUPTS DISABLED\n");
}

void z80_EI() {
   // EI Disables interrupts for one instruction, then enables them
   z80_delayed_enable_interrupt = 2;
   z80_interrupts_enabled       = false;
   z80_dt                       = 4;
}

// ADD / ADC / SUB / SUBC

void z80_ADD(byte inp) {
   SET_FLAG_N(false);
   SET_FLAG_H(((z80_A & 0x0F) + (inp & 0x0F)) > 0x0F);
   word temp_result = z80_A + inp;
   z80_A = temp_result & 0xFF;
   SET_FLAG_C(temp_result > 0xFF);
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_ADC(byte inp) {
   SET_FLAG_N(false);
   byte temp_result = z80_A + inp + GET_FLAG_C;
   SET_FLAG_H((z80_A & 0x0F) + (inp & 0x0F) + GET_FLAG_C > 0x0F);
   SET_FLAG_C(z80_A + inp + GET_FLAG_C > 0xFF);
   z80_A = temp_result;
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_SUB(byte inp) {
   SET_FLAG_N(true);
   SET_FLAG_H((z80_A & 0x0F) < (inp & 0x0F));
   SET_FLAG_C(z80_A < inp);
   z80_A -= inp;
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_SBC(byte inp) {
   SET_FLAG_N(true);
   byte temp_result = z80_A - inp - GET_FLAG_C;
   SET_FLAG_H((z80_A & 0x0F) < (inp & 0x0F) + GET_FLAG_C);
   SET_FLAG_C(z80_A < inp + GET_FLAG_C);
   z80_A = temp_result;
   SET_FLAG_Z(z80_A == 0);
   z80_dt = 4;
}

void z80_ADD_A_A() {
   z80_ADD(z80_A);
}

void z80_ADD_A_B() {
   z80_ADD(z80_B);
}

void z80_ADD_A_C() {
   z80_ADD(z80_C);
}

void z80_ADD_A_D() {
   z80_ADD(z80_D);
}

void z80_ADD_A_E() {
   z80_ADD(z80_E);
}

void z80_ADD_A_H() {
   z80_ADD(z80_H);
}

void z80_ADD_A_L() {
   z80_ADD(z80_L);
}

void z80_ADD_A_AT_HL() {
   z80_ADD(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_ADD_A_n() {
   z80_ADD(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_ADC_A_A() {
   z80_ADC(z80_A);
}

void z80_ADC_A_B() {
   z80_ADC(z80_B);
}

void z80_ADC_A_C() {
   z80_ADC(z80_C);
}

void z80_ADC_A_D() {
   z80_ADC(z80_D);
}

void z80_ADC_A_E() {
   z80_ADC(z80_E);
}

void z80_ADC_A_H() {
   z80_ADC(z80_H);
}

void z80_ADC_A_L() {
   z80_ADC(z80_L);
}

void z80_ADC_A_AT_HL() {
   z80_ADC(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_ADC_A_n() {
   z80_ADC(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_SUB_A_A() {
   z80_SUB(z80_A);
}

void z80_SUB_A_B() {
   z80_SUB(z80_B);
}

void z80_SUB_A_C() {
   z80_SUB(z80_C);
}

void z80_SUB_A_D() {
   z80_SUB(z80_D);
}

void z80_SUB_A_E() {
   z80_SUB(z80_E);
}

void z80_SUB_A_H() {
   z80_SUB(z80_H);
}

void z80_SUB_A_L() {
   z80_SUB(z80_L);
}

void z80_SUB_A_AT_HL() {
   z80_SUB(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_SUB_A_n() {
   z80_SUB(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_SBC_A_A() {
   z80_SBC(z80_A);
}

void z80_SBC_A_B() {
   z80_SBC(z80_B);
}

void z80_SBC_A_C() {
   z80_SBC(z80_C);
}

void z80_SBC_A_D() {
   z80_SBC(z80_D);
}

void z80_SBC_A_E() {
   z80_SBC(z80_E);
}

void z80_SBC_A_H() {
   z80_SBC(z80_H);
}

void z80_SBC_A_L() {
   z80_SBC(z80_L);
}

void z80_SBC_A_AT_HL() {
   z80_SBC(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_SBC_A_n() {
   z80_SBC(mem_rb(z80_PC++));
   z80_dt = 8;
}

// INCREMENT / DECREMENT

void z80_INC(byte* reg) {
   SET_FLAG_H((*reg & 0x0F) + 1 > 0x0F);
   SET_FLAG_N(false);
   (*reg)++;
   SET_FLAG_Z((*reg) == 0);
   z80_dt = 4;
}

void z80_DEC(byte* reg) {
   SET_FLAG_H((*reg & 0x0F) == 0);
   (*reg)--;
   SET_FLAG_Z(*reg == 0);
   SET_FLAG_N(true);
   z80_dt = 4;
}

void z80_INC_A() {
   z80_INC(&z80_A);
}

void z80_INC_B() {
   z80_INC(&z80_B);
}

void z80_INC_C() {
   z80_INC(&z80_C);
}

void z80_INC_D() {
   z80_INC(&z80_D);
}

void z80_INC_E() {
   z80_INC(&z80_E);
}

void z80_INC_H() {
   z80_INC(&z80_H);
}

void z80_INC_L() {
   z80_INC(&z80_L);
}

void z80_INC_AT_HL() {
   byte val = z80_FETCH(z80_H, z80_L);
   z80_INC(&val);
   z80_STORE(z80_H, z80_L, val);
   z80_dt = 12;
}

void z80_DEC_A() {
   z80_DEC(&z80_A);
}

void z80_DEC_B() {
   z80_DEC(&z80_B);
}

void z80_DEC_C() {
   z80_DEC(&z80_C);
}

void z80_DEC_D() {
   z80_DEC(&z80_D);
}

void z80_DEC_E() {
   z80_DEC(&z80_E);
}

void z80_DEC_H() {
   z80_DEC(&z80_H);
}

void z80_DEC_L() {
   z80_DEC(&z80_L);
}

void z80_DEC_AT_HL() {
   byte val = z80_FETCH(z80_H, z80_L);
   z80_DEC(&val);
   z80_STORE(z80_H, z80_L, val);
   z80_dt = 12;
}

// COMPARE

void z80_COMPARE(byte inp) {
   SET_FLAG_H((z80_A & 0x0F) < (inp & 0x0F));
   SET_FLAG_C(z80_A < inp);
   SET_FLAG_N(true);
   SET_FLAG_Z(z80_A == inp);
   z80_dt = 4;
}

void z80_CP_A() {
   z80_COMPARE(z80_A);
}

void z80_CP_B() {
   z80_COMPARE(z80_B);
}

void z80_CP_C() {
   z80_COMPARE(z80_C);
}

void z80_CP_D() {
   z80_COMPARE(z80_D);
}

void z80_CP_E() {
   z80_COMPARE(z80_E);
}

void z80_CP_H() {
   z80_COMPARE(z80_H);
}

void z80_CP_L() {
   z80_COMPARE(z80_L);
}

void z80_CP_AT_HL() {
   z80_COMPARE(z80_FETCH(z80_H, z80_L));
   z80_dt = 8;
}

void z80_CP_A_n() {
   z80_COMPARE(mem_rb(z80_PC++));
   z80_dt = 8;
}

void z80_CPL() {
   z80_A = ~z80_A;
   SET_FLAG_H(true);
   SET_FLAG_N(true);
   z80_dt = 4;
}

void z80_CCF() {
   SET_FLAG_C(!GET_FLAG_C);
   SET_FLAG_N(false);
   SET_FLAG_H(false);
   z80_dt = 4;
}

void z80_SCF() {
   SET_FLAG_H(false);
   SET_FLAG_N(false);
   SET_FLAG_C(true);
   z80_dt = 4;
}

void z80_HALT() {
   z80_halt = true;
   z80_dt   = 4;
   DEBUG("HALT\n");
}

void z80_STOP() {
   z80_dt   = 4;
   z80_stop = true;
   DEBUG("STOP\n");
}

void z80_DAA() {
   z80_dt = 4;
   word a = z80_A;
   if (!GET_FLAG_N) {
      if (GET_FLAG_H || (a & 0x0F) > 0x09) {
         a += 0x06;
      }
      if (GET_FLAG_C || a > 0x9F) {
         a += 0x60;
      }
   } else {
      if (GET_FLAG_H) {
         a = (a - 0x06) & 0xFF;
      }
      if (GET_FLAG_C) {
         a -= 0x60;
      }
   }

   SET_FLAG_H(false);
   SET_FLAG_C(a & 0x0100);
   SET_FLAG_Z(a == 0);
   z80_A = 0xFF & a;
}
