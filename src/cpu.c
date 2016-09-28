#include "cpu.h"
#include "ppu.h"

// Opcode macros

#define LOAD(reg, val) \
   reg = (val); 

// rb and wb advance time by 4 ticks each
#define STORE(hi, lo, val) \
   mem_wb((((hi) & 0xFF) << 8) | ((lo) & 0xFF), (val)); 

#define FETCH(hi, lo) \
   mem_rb((((hi) & 0xFF) << 8) | ((lo) & 0xFF))

#define AND(val) \
   cpu_A = cpu_A & (val); \
   FLAG_C = FLAG_N = false; \
   FLAG_Z = !cpu_A; \
   FLAG_H = true; 

#define OR(val) \
   cpu_A = cpu_A | (val); \
   FLAG_C = FLAG_H = FLAG_N = false; \
   FLAG_Z = !cpu_A; 

#define XOR(val) \
   cpu_A = cpu_A ^ (val); \
   FLAG_C = FLAG_H = FLAG_N = false; \
   FLAG_Z = !cpu_A; 

#define PUSH(hi, lo) \
   mem_direct_write(--cpu_SP, (hi)); \
   mem_direct_write(--cpu_SP, (lo)); \
   cpu_advance_time(12);

#define POP(hi, lo) \
   lo = mem_direct_read(cpu_SP++); \
   hi = mem_direct_read(cpu_SP++); \
   cpu_advance_time(8);

#define PUSHW(val) \
   cpu_SP -= 2; \
   mem_ww(cpu_SP, (val)); 

#define POPW(val) \
   val = mem_rw(cpu_SP); \
   cpu_SP += 2; \

#define JP() \
   cpu_PC = mem_rw(cpu_PC); \
   cpu_advance_time(4);

#define JR() \
   cpu_PC += (sbyte)mem_rb(cpu_PC) + 1; \
   cpu_advance_time(4);

#define ADD(val) \
   byte v = (val); \
   FLAG_N = false; \
   FLAG_H = ((cpu_A & 0x0F) + (v & 0x0F)) > 0x0F; \
   word tmp = cpu_A + v; \
   cpu_A = tmp & 0xFF; \
   FLAG_C = tmp > 0xFF; \
   FLAG_Z = cpu_A == 0; 

#define ADC(val) \
   byte v = (val); \
   FLAG_N = false; \
   word tmp = cpu_A + v + FLAG_C; \
   FLAG_H = (cpu_A & 0x0F) + (v & 0x0F) + FLAG_C > 0x0F; \
   FLAG_C = tmp > 0xFF; \
   cpu_A = tmp & 0xFF; \
   FLAG_Z = cpu_A == 0; 

#define SUB(val) \
   byte v = (val); \
   FLAG_N = true; \
   FLAG_H = (cpu_A & 0x0F) < (v & 0x0F); \
   FLAG_C = cpu_A < v; \
   cpu_A -= v; \
   FLAG_Z = cpu_A == 0; 

#define SBC(val) \
   byte v = (val); \
   FLAG_N = true; \
   byte tmp = cpu_A - v - FLAG_C; \
   FLAG_H = (cpu_A & 0x0F) < (v & 0x0F) + FLAG_C; \
   FLAG_C = cpu_A < v + FLAG_C; \
   cpu_A = tmp; \
   FLAG_Z = cpu_A == 0; 

#define INC(dst) \
   FLAG_H = ((dst) & 0x0F) + 1 > 0x0F; \
   FLAG_N = false; \
   dst++; \
   FLAG_Z = (dst) == 0; 

#define DEC(dst) \
   FLAG_H = ((dst) & 0x0F) == 0; \
   dst--; \
   FLAG_Z = (dst) == 0; \
   FLAG_N = true; 

#define COMPARE(val) \
   byte v = (val); \
   FLAG_H = (cpu_A & 0x0F) < (v & 0x0F); \
   FLAG_C = cpu_A < v; \
   FLAG_N = true; \
   FLAG_Z = cpu_A == v; 

#define CLEAR_FLAGS() \
   FLAG_C = false; \
   FLAG_H = false; \
   FLAG_Z = false; \
   FLAG_N = false;

// Flag register

static bool FLAG_C;
static bool FLAG_H;
static bool FLAG_Z;
static bool FLAG_N;

tick ticks = 0;
void cpu_update_timers();
   
void cpu_init(char* romname) {
   cpu_ei         = false;
   cpu_ei_delay   = false;
   cpu_tima_timer = 1024;
   cpu_div_timer  = 0;
   cpu_halt       = false;
   cpu_stop       = false;
   cpu_rom_fname  = romname;

   for (size_t i = 0; i < 0x100; i++) {
      cpu_opcodes[i] = &cpu_NI;
   }

                                           // Timing

   cpu_opcodes[0x00] = &cpu_NOP;           /* 1 */ 
   cpu_opcodes[0x01] = &cpu_LDBC_nn;       /* 3 */
   cpu_opcodes[0x02] = &cpu_LD_AT_BC_A;    /* 2 */
   cpu_opcodes[0x03] = &cpu_INC16_BC;      /* 2 */
   cpu_opcodes[0x04] = &cpu_INC_B;         /* 1 */
   cpu_opcodes[0x05] = &cpu_DEC_B;         /* 1 */
   cpu_opcodes[0x06] = &cpu_LDB_n;         /* 2 */
   cpu_opcodes[0x07] = &cpu_RLCA;          /* 1 */
   cpu_opcodes[0x08] = &cpu_LD_nn_SP;      /* 5 */
   cpu_opcodes[0x09] = &cpu_ADD16_HL_BC;   /* 2 */
   cpu_opcodes[0x0A] = &cpu_LDA_AT_BC;     /* 2 */
   cpu_opcodes[0x0B] = &cpu_DEC16_BC;      /* 2 */
   cpu_opcodes[0x0C] = &cpu_INC_C;         /* 1 */
   cpu_opcodes[0x0D] = &cpu_DEC_C;         /* 1 */
   cpu_opcodes[0x0E] = &cpu_LDC_n;         /* 2 */
   cpu_opcodes[0x0F] = &cpu_RRCA;          /* 1 */
                                           
   cpu_opcodes[0x10] = &cpu_STOP;          /* 0 */
   cpu_opcodes[0x11] = &cpu_LDDE_nn;       /* 3 */
   cpu_opcodes[0x12] = &cpu_LD_AT_DE_A;    /* 2 */
   cpu_opcodes[0x13] = &cpu_INC16_DE;      /* 2 */
   cpu_opcodes[0x14] = &cpu_INC_D;         /* 1 */
   cpu_opcodes[0x15] = &cpu_DEC_D;         /* 1 */
   cpu_opcodes[0x16] = &cpu_LDD_n;         /* 2 */
   cpu_opcodes[0x17] = &cpu_RLA;           /* 1 */
   cpu_opcodes[0x18] = &cpu_JR_n;          /* 3 */
   cpu_opcodes[0x19] = &cpu_ADD16_HL_DE;   /* 2 */
   cpu_opcodes[0x1A] = &cpu_LDA_AT_DE;     /* 2 */
   cpu_opcodes[0x1B] = &cpu_DEC16_DE;      /* 2 */
   cpu_opcodes[0x1C] = &cpu_INC_E;         /* 1 */
   cpu_opcodes[0x1D] = &cpu_DEC_E;         /* 1 */
   cpu_opcodes[0x1E] = &cpu_LDE_n;         /* 2 */
   cpu_opcodes[0x1F] = &cpu_RRA;           /* 1 */
                                           
   cpu_opcodes[0x20] = &cpu_JR_NZ_n;       /* 2 */
   cpu_opcodes[0x21] = &cpu_LDHL_nn;       /* 3 */
   cpu_opcodes[0x22] = &cpu_LD_AT_HLI_A;   /* 2 */
   cpu_opcodes[0x23] = &cpu_INC16_HL;      /* 2 */
   cpu_opcodes[0x24] = &cpu_INC_H;         /* 1 */
   cpu_opcodes[0x25] = &cpu_DEC_H;         /* 1 */
   cpu_opcodes[0x26] = &cpu_LDH_n;         /* 2 */
   cpu_opcodes[0x27] = &cpu_DAA;           /* 1 */
   cpu_opcodes[0x28] = &cpu_JR_Z_n;        /* 2 */
   cpu_opcodes[0x29] = &cpu_ADD16_HL_HL;   /* 2 */
   cpu_opcodes[0x2A] = &cpu_LDA_AT_HLI;    /* 2 */
   cpu_opcodes[0x2B] = &cpu_DEC16_HL;      /* 2 */
   cpu_opcodes[0x2C] = &cpu_INC_L;         /* 1 */
   cpu_opcodes[0x2D] = &cpu_DEC_L;         /* 1 */
   cpu_opcodes[0x2E] = &cpu_LDL_n;         /* 2 */
   cpu_opcodes[0x2F] = &cpu_CPL;           /* 1 */
                                           
   cpu_opcodes[0x30] = &cpu_JR_NC_n;       /* 2 */
   cpu_opcodes[0x31] = &cpu_LDSP_nn;       /* 3 */
   cpu_opcodes[0x32] = &cpu_LD_AT_HLD_A;   /* 2 */
   cpu_opcodes[0x33] = &cpu_INC16_SP;      /* 2 */
   cpu_opcodes[0x34] = &cpu_INC_AT_HL;     /* 3 */
   cpu_opcodes[0x35] = &cpu_DEC_AT_HL;     /* 3 */
   cpu_opcodes[0x36] = &cpu_LD_AT_HL_n;    /* 3 */
   cpu_opcodes[0x37] = &cpu_SCF;           /* 1 */
   cpu_opcodes[0x38] = &cpu_JR_C_n;        /* 2 */
   cpu_opcodes[0x39] = &cpu_ADD16_HL_SP;   /* 2 */
   cpu_opcodes[0x3A] = &cpu_LDA_AT_HLD;    /* 2 */
   cpu_opcodes[0x3B] = &cpu_DEC16_SP;      /* 2 */
   cpu_opcodes[0x3C] = &cpu_INC_A;         /* 1 */
   cpu_opcodes[0x3D] = &cpu_DEC_A;         /* 1 */
   cpu_opcodes[0x3E] = &cpu_LDA_n;         /* 2 */
   cpu_opcodes[0x3F] = &cpu_CCF;           /* 1 */
                                           
   cpu_opcodes[0x40] = &cpu_LDB_B;         /* 1 */
   cpu_opcodes[0x41] = &cpu_LDB_C;         /* 1 */
   cpu_opcodes[0x42] = &cpu_LDB_D;         /* 1 */
   cpu_opcodes[0x43] = &cpu_LDB_E;         /* 1 */
   cpu_opcodes[0x44] = &cpu_LDB_H;         /* 1 */
   cpu_opcodes[0x45] = &cpu_LDB_L;         /* 1 */
   cpu_opcodes[0x46] = &cpu_LDB_AT_HL;     /* 2 */
   cpu_opcodes[0x47] = &cpu_LDB_A;         /* 1 */
   cpu_opcodes[0x48] = &cpu_LDC_B;         /* 1 */
   cpu_opcodes[0x49] = &cpu_LDC_C;         /* 1 */
   cpu_opcodes[0x4A] = &cpu_LDC_D;         /* 1 */
   cpu_opcodes[0x4B] = &cpu_LDC_E;         /* 1 */
   cpu_opcodes[0x4C] = &cpu_LDC_H;         /* 1 */
   cpu_opcodes[0x4D] = &cpu_LDC_L;         /* 1 */
   cpu_opcodes[0x4E] = &cpu_LDC_AT_HL;     /* 2 */
   cpu_opcodes[0x4F] = &cpu_LDC_A;         /* 1 */
                                           
   cpu_opcodes[0x50] = &cpu_LDD_B;         /* 1 */
   cpu_opcodes[0x51] = &cpu_LDD_C;         /* 1 */
   cpu_opcodes[0x52] = &cpu_LDD_D;         /* 1 */
   cpu_opcodes[0x53] = &cpu_LDD_E;         /* 1 */
   cpu_opcodes[0x54] = &cpu_LDD_H;         /* 1 */
   cpu_opcodes[0x55] = &cpu_LDD_L;         /* 1 */
   cpu_opcodes[0x56] = &cpu_LDD_AT_HL;     /* 2 */
   cpu_opcodes[0x57] = &cpu_LDD_A;         /* 1 */
   cpu_opcodes[0x58] = &cpu_LDE_B;         /* 1 */
   cpu_opcodes[0x59] = &cpu_LDE_C;         /* 1 */
   cpu_opcodes[0x5A] = &cpu_LDE_D;         /* 1 */
   cpu_opcodes[0x5B] = &cpu_LDE_E;         /* 1 */
   cpu_opcodes[0x5C] = &cpu_LDE_H;         /* 1 */
   cpu_opcodes[0x5D] = &cpu_LDE_L;         /* 1 */
   cpu_opcodes[0x5E] = &cpu_LDE_AT_HL;     /* 2 */
   cpu_opcodes[0x5F] = &cpu_LDE_A;         /* 1 */
                                           
   cpu_opcodes[0x60] = &cpu_LDH_B;         /* 1 */
   cpu_opcodes[0x61] = &cpu_LDH_C;         /* 1 */
   cpu_opcodes[0x62] = &cpu_LDH_D;         /* 1 */
   cpu_opcodes[0x63] = &cpu_LDH_E;         /* 1 */
   cpu_opcodes[0x64] = &cpu_LDH_H;         /* 1 */
   cpu_opcodes[0x65] = &cpu_LDH_L;         /* 1 */
   cpu_opcodes[0x66] = &cpu_LDH_AT_HL;     /* 2 */
   cpu_opcodes[0x67] = &cpu_LDH_A;         /* 1 */
   cpu_opcodes[0x68] = &cpu_LDL_B;         /* 1 */
   cpu_opcodes[0x69] = &cpu_LDL_C;         /* 1 */
   cpu_opcodes[0x6A] = &cpu_LDL_D;         /* 1 */
   cpu_opcodes[0x6B] = &cpu_LDL_E;         /* 1 */
   cpu_opcodes[0x6C] = &cpu_LDL_H;         /* 1 */
   cpu_opcodes[0x6D] = &cpu_LDL_L;         /* 1 */
   cpu_opcodes[0x6E] = &cpu_LDL_AT_HL;     /* 2 */
   cpu_opcodes[0x6F] = &cpu_LDL_A;         /* 1 */
                                           
   cpu_opcodes[0x70] = &cpu_LD_AT_HL_B;    /* 2 */
   cpu_opcodes[0x71] = &cpu_LD_AT_HL_C;    /* 2 */
   cpu_opcodes[0x72] = &cpu_LD_AT_HL_D;    /* 2 */
   cpu_opcodes[0x73] = &cpu_LD_AT_HL_E;    /* 2 */
   cpu_opcodes[0x74] = &cpu_LD_AT_HL_H;    /* 2 */
   cpu_opcodes[0x75] = &cpu_LD_AT_HL_L;    /* 2 */
   cpu_opcodes[0x76] = &cpu_HALT;          /* 0 */
   cpu_opcodes[0x77] = &cpu_LD_AT_HL_A;    /* 2 */
   cpu_opcodes[0x78] = &cpu_LDA_B;         /* 1 */
   cpu_opcodes[0x79] = &cpu_LDA_C;         /* 1 */
   cpu_opcodes[0x7A] = &cpu_LDA_D;         /* 1 */
   cpu_opcodes[0x7B] = &cpu_LDA_E;         /* 1 */
   cpu_opcodes[0x7C] = &cpu_LDA_H;         /* 1 */
   cpu_opcodes[0x7D] = &cpu_LDA_L;         /* 1 */
   cpu_opcodes[0x7E] = &cpu_LDA_AT_HL;     /* 2 */
   cpu_opcodes[0x7F] = &cpu_LDA_A;         /* 1 */
                                           
   cpu_opcodes[0x80] = &cpu_ADD_A_B;       /* 1 */
   cpu_opcodes[0x81] = &cpu_ADD_A_C;       /* 1 */
   cpu_opcodes[0x82] = &cpu_ADD_A_D;       /* 1 */
   cpu_opcodes[0x83] = &cpu_ADD_A_E;       /* 1 */
   cpu_opcodes[0x84] = &cpu_ADD_A_H;       /* 1 */
   cpu_opcodes[0x85] = &cpu_ADD_A_L;       /* 1 */
   cpu_opcodes[0x86] = &cpu_ADD_A_AT_HL;   /* 2 */
   cpu_opcodes[0x87] = &cpu_ADD_A_A;       /* 1 */
   cpu_opcodes[0x88] = &cpu_ADC_A_B;       /* 1 */
   cpu_opcodes[0x89] = &cpu_ADC_A_C;       /* 1 */
   cpu_opcodes[0x8A] = &cpu_ADC_A_D;       /* 1 */
   cpu_opcodes[0x8B] = &cpu_ADC_A_E;       /* 1 */
   cpu_opcodes[0x8C] = &cpu_ADC_A_H;       /* 1 */
   cpu_opcodes[0x8D] = &cpu_ADC_A_L;       /* 1 */
   cpu_opcodes[0x8E] = &cpu_ADC_A_AT_HL;   /* 2 */
   cpu_opcodes[0x8F] = &cpu_ADC_A_A;       /* 1 */
                                           
   cpu_opcodes[0x90] = &cpu_SUB_A_B;       /* 1 */
   cpu_opcodes[0x91] = &cpu_SUB_A_C;       /* 1 */
   cpu_opcodes[0x92] = &cpu_SUB_A_D;       /* 1 */
   cpu_opcodes[0x93] = &cpu_SUB_A_E;       /* 1 */
   cpu_opcodes[0x94] = &cpu_SUB_A_H;       /* 1 */
   cpu_opcodes[0x95] = &cpu_SUB_A_L;       /* 1 */
   cpu_opcodes[0x96] = &cpu_SUB_A_AT_HL;   /* 2 */
   cpu_opcodes[0x97] = &cpu_SUB_A_A;       /* 1 */
   cpu_opcodes[0x98] = &cpu_SBC_A_B;       /* 1 */
   cpu_opcodes[0x99] = &cpu_SBC_A_C;       /* 1 */
   cpu_opcodes[0x9A] = &cpu_SBC_A_D;       /* 1 */
   cpu_opcodes[0x9B] = &cpu_SBC_A_E;       /* 1 */
   cpu_opcodes[0x9C] = &cpu_SBC_A_H;       /* 1 */
   cpu_opcodes[0x9D] = &cpu_SBC_A_L;       /* 1 */
   cpu_opcodes[0x9E] = &cpu_SBC_A_AT_HL;   /* 2 */
   cpu_opcodes[0x9F] = &cpu_SBC_A_A;       /* 1 */
                                           
   cpu_opcodes[0xA0] = &cpu_AND_B;         /* 1 */
   cpu_opcodes[0xA1] = &cpu_AND_C;         /* 1 */
   cpu_opcodes[0xA2] = &cpu_AND_D;         /* 1 */
   cpu_opcodes[0xA3] = &cpu_AND_E;         /* 1 */
   cpu_opcodes[0xA4] = &cpu_AND_H;         /* 1 */
   cpu_opcodes[0xA5] = &cpu_AND_L;         /* 1 */
   cpu_opcodes[0xA6] = &cpu_AND_AT_HL;     /* 2 */
   cpu_opcodes[0xA7] = &cpu_AND_A;         /* 1 */
   cpu_opcodes[0xA8] = &cpu_XOR_B;         /* 1 */
   cpu_opcodes[0xA9] = &cpu_XOR_C;         /* 1 */
   cpu_opcodes[0xAA] = &cpu_XOR_D;         /* 1 */
   cpu_opcodes[0xAB] = &cpu_XOR_E;         /* 1 */
   cpu_opcodes[0xAC] = &cpu_XOR_H;         /* 1 */
   cpu_opcodes[0xAD] = &cpu_XOR_L;         /* 1 */
   cpu_opcodes[0xAE] = &cpu_XOR_AT_HL;     /* 2 */
   cpu_opcodes[0xAF] = &cpu_XOR_A;         /* 1 */
                                           
   cpu_opcodes[0xB0] = &cpu_OR_B;          /* 1 */
   cpu_opcodes[0xB1] = &cpu_OR_C;          /* 1 */
   cpu_opcodes[0xB2] = &cpu_OR_D;          /* 1 */
   cpu_opcodes[0xB3] = &cpu_OR_E;          /* 1 */
   cpu_opcodes[0xB4] = &cpu_OR_H;          /* 1 */
   cpu_opcodes[0xB5] = &cpu_OR_L;          /* 1 */
   cpu_opcodes[0xB6] = &cpu_OR_AT_HL;      /* 2 */
   cpu_opcodes[0xB7] = &cpu_OR_A;          /* 1 */
   cpu_opcodes[0xB8] = &cpu_CP_B;          /* 1 */
   cpu_opcodes[0xB9] = &cpu_CP_C;          /* 1 */
   cpu_opcodes[0xBA] = &cpu_CP_D;          /* 1 */
   cpu_opcodes[0xBB] = &cpu_CP_E;          /* 1 */
   cpu_opcodes[0xBC] = &cpu_CP_H;          /* 1 */
   cpu_opcodes[0xBD] = &cpu_CP_L;          /* 1 */
   cpu_opcodes[0xBE] = &cpu_CP_AT_HL;      /* 2 */
   cpu_opcodes[0xBF] = &cpu_CP_A;          /* 1 */
                                           
   cpu_opcodes[0xC0] = &cpu_RET_NZ;        /* 2 */
   cpu_opcodes[0xC1] = &cpu_POPBC;         /* 3 */
   cpu_opcodes[0xC2] = &cpu_JP_NZ_nn;      /* 3 */
   cpu_opcodes[0xC3] = &cpu_JP_nn;         /* 4 */
   cpu_opcodes[0xC4] = &cpu_CALL_NZ_nn;    /* 3 */
   cpu_opcodes[0xC5] = &cpu_PUSHBC;        /* 4 */
   cpu_opcodes[0xC6] = &cpu_ADD_A_n;       /* 2 */
   cpu_opcodes[0xC7] = &cpu_RST_00H;       /* 4 */
   cpu_opcodes[0xC8] = &cpu_RET_Z;         /* 2 */
   cpu_opcodes[0xC9] = &cpu_RET;           /* 4 */
   cpu_opcodes[0xCA] = &cpu_JP_Z_nn;       /* 3 */
   cpu_opcodes[0xCB] = &cpu_CB;            /* 0 */
   cpu_opcodes[0xCC] = &cpu_CALL_Z_nn;     /* 3 */
   cpu_opcodes[0xCD] = &cpu_CALL_nn;       /* 6 */
   cpu_opcodes[0xCE] = &cpu_ADC_A_n;       /* 2 */
   cpu_opcodes[0xCF] = &cpu_RST_08H;       /* 4 */
                                           
   cpu_opcodes[0xD0] = &cpu_RET_NC;        /* 2 */
   cpu_opcodes[0xD1] = &cpu_POPDE;         /* 3 */
   cpu_opcodes[0xD2] = &cpu_JP_NC_nn;      /* 3 */
   cpu_opcodes[0xD3] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xD4] = &cpu_CALL_NC_nn;    /* 3 */
   cpu_opcodes[0xD5] = &cpu_PUSHDE;        /* 4 */
   cpu_opcodes[0xD6] = &cpu_SUB_A_n;       /* 2 */
   cpu_opcodes[0xD7] = &cpu_RST_10H;       /* 4 */
   cpu_opcodes[0xD8] = &cpu_RET_C;         /* 2 */
   cpu_opcodes[0xD9] = &cpu_RETI;          /* 4 */
   cpu_opcodes[0xDA] = &cpu_JP_C_nn;       /* 3 */
   cpu_opcodes[0xDB] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xDC] = &cpu_CALL_C_nn;     /* 3 */
   cpu_opcodes[0xDD] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xDE] = &cpu_SBC_A_n;       /* 2 */
   cpu_opcodes[0xDF] = &cpu_RST_18H;       /* 4 */
                                           
   cpu_opcodes[0xE0] = &cpu_LD_n_A;        /* 3 */
   cpu_opcodes[0xE1] = &cpu_POPHL;         /* 3 */
   cpu_opcodes[0xE2] = &cpu_LD_AT_C_A;     /* 2 */
   cpu_opcodes[0xE3] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xE4] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xE5] = &cpu_PUSHHL;        /* 4 */
   cpu_opcodes[0xE6] = &cpu_AND_n;         /* 2 */
   cpu_opcodes[0xE7] = &cpu_RST_20H;       /* 4 */
   cpu_opcodes[0xE8] = &cpu_ADD16_SP_n;    /* 4 */
   cpu_opcodes[0xE9] = &cpu_JP_AT_HL;      /* 1 */
   cpu_opcodes[0xEA] = &cpu_LD_AT_nn_A;    /* 4 */
   cpu_opcodes[0xEB] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xEC] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xED] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xEE] = &cpu_XOR_n;         /* 2 */
   cpu_opcodes[0xEF] = &cpu_RST_28H;       /* 4 */
                                           
  cpu_opcodes[0xF0] = &cpu_LD_A_n;        /* 3 */
   cpu_opcodes[0xF1] = &cpu_POPAF;         /* 3 */
   cpu_opcodes[0xF2] = &cpu_LDA_AT_C;      /* 2 */
   cpu_opcodes[0xF3] = &cpu_DI;            /* 1 */
   cpu_opcodes[0xF4] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xF5] = &cpu_PUSHAF;        /* 4 */
   cpu_opcodes[0xF6] = &cpu_OR_n;          /* 2 */
   cpu_opcodes[0xF7] = &cpu_RST_30H;       /* 4 */
   cpu_opcodes[0xF8] = &cpu_LDHL_SP_n;     /* 3 */
   cpu_opcodes[0xF9] = &cpu_LDSP_HL;       /* 2 */
   cpu_opcodes[0xFA] = &cpu_LDA_AT_nn;     /* 4 */
   cpu_opcodes[0xFB] = &cpu_EI;            /* 1 */
   cpu_opcodes[0xFC] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xFD] = &cpu_NI;            /* 0 */
   cpu_opcodes[0xFE] = &cpu_CP_A_n;        /* 2 */
   cpu_opcodes[0xFF] = &cpu_RST_38H;       /* 4 */

   cpu_reset();
}

void cpu_reset() {
   cpu_PC    = 0x0100;
   cpu_A     = 0x01;
   cpu_B     = 0x00;
   cpu_C     = 0x13;
   cpu_D     = 0x00;
   cpu_E     = 0xD8;
   cpu_SP    = 0xFFFE;
   cpu_H     = 0x01;
   cpu_L     = 0x4D;
   cpu_ticks = 0;

   FLAG_C = true;
   FLAG_H = true;
   FLAG_Z = true;
   FLAG_N = false;

   mem_init();

   // Setup our in-memory registers
   mem_direct_write(INPUT_REGISTER_ADDR, 0xFF);
   mem_direct_write(DIV_REGISTER_ADDR, 0xAF);
   mem_direct_write(TIMER_CONTROL_ADDR, 0xF8);
   mem_direct_write(0xFF10, 0x80);
   mem_direct_write(0xFF11, 0xBF);
   mem_direct_write(0xFF12, 0xF3);
   mem_direct_write(0xFF14, 0xBF);
   mem_direct_write(0xFF16, 0x3F);
   mem_direct_write(0xFF19, 0xBF);
   mem_direct_write(0xFF1A, 0x7F);
   mem_direct_write(0xFF1B, 0xFF);
   mem_direct_write(0xFF1C, 0x9F);
   mem_direct_write(0xFF1E, 0xBF);
   mem_direct_write(0xFF20, 0xFF);
   mem_direct_write(0xFF23, 0xBF);
   mem_direct_write(0xFF24, 0x77);
   mem_direct_write(0xFF25, 0xF3);
   mem_direct_write(0xFF26, 0xF1);
   mem_direct_write(LCD_CONTROL_ADDR, 0x83);
   mem_direct_write(0xFF47, 0xFC);
   mem_direct_write(0xFF48, 0xFF);
   mem_direct_write(0xFF49, 0xFF);

   mem_load_image(cpu_rom_fname);
   mem_get_rom_info();
}

void cpu_advance_time(tick dt) {
   ticks += dt;
   cpu_ticks += dt;
}

void cpu_update_timers() {
   tick dt = ticks;
   ticks = 0;
   cpu_div_timer += dt;

   if (cpu_div_timer >= 0xFF) {
      cpu_div_timer = 0;
      mem_direct_write(DIV_REGISTER_ADDR,
            mem_direct_read(DIV_REGISTER_ADDR) + 1);
   }

   // Bit 3 enables or disables timers
   if (mem_direct_read(TIMER_CONTROL_ADDR) & 0x04) {
      cpu_tima_timer -= dt;
      int max = 0;
      // Bits 1-2 control the timer speed
      while (cpu_tima_timer <= 0) { 
         switch (mem_direct_read(TIMER_CONTROL_ADDR) & 0x3) {
            case 0: max = 1024; break;
            case 1: max = 16; break;
            case 2: max = 64; break;
            case 3: max = 256; break;
            default: ERROR("timer error");
         }
         cpu_tima_timer += max;
         // Technically this interrupt happens 4 cycles after the overflow.
         // TODO: Delay this
         mem_direct_write(TIMA_ADDR,
                          mem_direct_read(TIMA_ADDR) + 1);
         if (mem_direct_read(TIMA_ADDR) == 0) {
            DEBUG("TIMA OVERFLOW (MOD %02X)\n",
                  mem_direct_read(TMA_ADDR));
            mem_direct_write(INT_FLAG_ADDR,
                             mem_direct_read(INT_FLAG_ADDR) | INT_TIMA);
            mem_direct_write(TIMA_ADDR, mem_direct_read(TMA_ADDR));
         }
      }
   }
   ppu_advance_time(dt);
}

void cpu_execute_step() {
   // Check interrupts
   bool interrupted = false;
   byte int_IE = mem_direct_read(INT_ENABLED_ADDR);
   byte int_IF = mem_direct_read(INT_FLAG_ADDR);
   byte irq    = int_IE & int_IF;

   if (int_IF != 0 && cpu_halt) {
      DEBUG("IRQ ENDED HALT\n");
      cpu_halt = false;
   }

   //if (int_IF & INT_INPUT) {
   if (int_IF != 0 && cpu_stop) {
      DEBUG("IRQ ENDED STOP\n");
      cpu_stop = false;
   }

   if (cpu_ei) {
      if (!cpu_ei_delay) {
         byte target = 0x00;
         if ((irq & INT_VBLANK) != 0) {
            target = 0x40;
            mem_direct_write(INT_FLAG_ADDR, int_IF & ~INT_VBLANK);
            DEBUG("VBLANK INTERRUPT\n");
         } else if ((irq & INT_STAT) != 0) {
            target = 0x48;
            mem_direct_write(INT_FLAG_ADDR, int_IF & ~INT_STAT);
            DEBUG("STAT INTERRUPT\n");
         } else if ((irq & INT_TIMA) != 0) {
            target = 0x50;
            mem_direct_write(INT_FLAG_ADDR, int_IF & ~INT_TIMA);
            DEBUG("TIMA INTERRUPT\n");
         } else if ((irq & INT_INPUT) != 0) {
            target = 0x60;
            mem_direct_write(INT_FLAG_ADDR, int_IF & ~INT_INPUT);
            DEBUG("INPUT INTERRUPT\n");
         }

         if (target != 0x00) {
            interrupted = true;
            cpu_ei = false;
            PUSHW(cpu_PC);
            cpu_PC = target;
            cpu_advance_time(12);
         }
      } else {
         cpu_ei_delay = false;
      }
   }

   if (!interrupted) {
      if (!cpu_halt && !cpu_stop) {
         cpu_last_pc = cpu_PC;
         // We already spend 4 tickcs reading the PC,
         // so instructions take 4 ticks less than listed in docs
         cpu_last_op = mem_rb(cpu_PC++);

         if (get_debug_time() > DEBUG_RANGE_BEGIN
          && get_debug_time() < DEBUG_RANGE_END) {
            DEBUG("\tA:%2X B:%2X C:%2X D:%2X E:%2X H:%02X L:%02X SP:%4X"
                  "\tC:%X H:%X N:%X Z:%X\n",
                  cpu_A, cpu_B, cpu_C, cpu_D, cpu_E, cpu_H, cpu_L, cpu_SP,
                  FLAG_C, FLAG_H, FLAG_N, FLAG_Z);
         }

         (*cpu_opcodes[cpu_last_op])();
      } else {
         cpu_advance_time(4);
      }
   }
   cpu_update_timers();
}

void cpu_NI() {
   ERROR("Invalid instruction at PC %04X, potentially opcode %02X\n",
         cpu_last_pc, cpu_last_op);
}

void cpu_NOP() { }

// LOAD / STORES
void cpu_LDA_n()      { LOAD(cpu_A, mem_rb(cpu_PC++)); } 
void cpu_LDB_n()      { LOAD(cpu_B, mem_rb(cpu_PC++)); }
void cpu_LDC_n()      { LOAD(cpu_C, mem_rb(cpu_PC++)); }
void cpu_LDD_n()      { LOAD(cpu_D, mem_rb(cpu_PC++)); }
void cpu_LDE_n()      { LOAD(cpu_E, mem_rb(cpu_PC++)); }
void cpu_LDH_n()      { LOAD(cpu_H, mem_rb(cpu_PC++)); }
void cpu_LDL_n()      { LOAD(cpu_L, mem_rb(cpu_PC++)); }
void cpu_LDA_A()      { LOAD(cpu_A, cpu_A); }
void cpu_LDA_B()      { LOAD(cpu_A, cpu_B); }
void cpu_LDA_C()      { LOAD(cpu_A, cpu_C); }
void cpu_LDA_D()      { LOAD(cpu_A, cpu_D); }
void cpu_LDA_E()      { LOAD(cpu_A, cpu_E); }
void cpu_LDA_H()      { LOAD(cpu_A, cpu_H); }
void cpu_LDA_L()      { LOAD(cpu_A, cpu_L); }
void cpu_LDA_AT_HL()  { LOAD(cpu_A, FETCH(cpu_H, cpu_L)); }
void cpu_LDB_A()      { LOAD(cpu_B, cpu_A); }
void cpu_LDB_B()      { LOAD(cpu_B, cpu_B); }
void cpu_LDB_C()      { LOAD(cpu_B, cpu_C); }
void cpu_LDB_D()      { LOAD(cpu_B, cpu_D); }
void cpu_LDB_E()      { LOAD(cpu_B, cpu_E); }
void cpu_LDB_H()      { LOAD(cpu_B, cpu_H); }
void cpu_LDB_L()      { LOAD(cpu_B, cpu_L); }
void cpu_LDB_AT_HL()  { LOAD(cpu_B, FETCH(cpu_H, cpu_L)); }
void cpu_LDC_A()      { LOAD(cpu_C, cpu_A); }
void cpu_LDC_B()      { LOAD(cpu_C, cpu_B); }
void cpu_LDC_C()      { LOAD(cpu_C, cpu_C); }
void cpu_LDC_D()      { LOAD(cpu_C, cpu_D); }
void cpu_LDC_E()      { LOAD(cpu_C, cpu_E); }
void cpu_LDC_H()      { LOAD(cpu_C, cpu_H); }
void cpu_LDC_L()      { LOAD(cpu_C, cpu_L); }
void cpu_LDC_AT_HL()  { LOAD(cpu_C, FETCH(cpu_H, cpu_L)); }
void cpu_LDD_A()      { LOAD(cpu_D, cpu_A); }
void cpu_LDD_B()      { LOAD(cpu_D, cpu_B); }
void cpu_LDD_C()      { LOAD(cpu_D, cpu_C); }
void cpu_LDD_D()      { LOAD(cpu_D, cpu_D); }
void cpu_LDD_E()      { LOAD(cpu_D, cpu_E); }
void cpu_LDD_H()      { LOAD(cpu_D, cpu_H); }
void cpu_LDD_L()      { LOAD(cpu_D, cpu_L); }
void cpu_LDD_AT_HL()  { LOAD(cpu_D, FETCH(cpu_H, cpu_L)); }
void cpu_LDE_A()      { LOAD(cpu_E, cpu_A); }
void cpu_LDE_B()      { LOAD(cpu_E, cpu_B); }
void cpu_LDE_C()      { LOAD(cpu_E, cpu_C); }
void cpu_LDE_D()      { LOAD(cpu_E, cpu_D); }
void cpu_LDE_E()      { LOAD(cpu_E, cpu_E); }
void cpu_LDE_H()      { LOAD(cpu_E, cpu_H); }
void cpu_LDE_L()      { LOAD(cpu_E, cpu_L); }
void cpu_LDE_AT_HL()  { LOAD(cpu_E, FETCH(cpu_H, cpu_L)); }
void cpu_LDH_A()      { LOAD(cpu_H, cpu_A); }
void cpu_LDH_B()      { LOAD(cpu_H, cpu_B); }
void cpu_LDH_C()      { LOAD(cpu_H, cpu_C); }
void cpu_LDH_D()      { LOAD(cpu_H, cpu_D); }
void cpu_LDH_E()      { LOAD(cpu_H, cpu_E); }
void cpu_LDH_H()      { LOAD(cpu_H, cpu_H); }
void cpu_LDH_L()      { LOAD(cpu_H, cpu_L); }
void cpu_LDH_AT_HL()  { LOAD(cpu_H, FETCH(cpu_H, cpu_L)); }
void cpu_LDL_A()      { LOAD(cpu_L, cpu_A); }
void cpu_LDL_B()      { LOAD(cpu_L, cpu_B); }
void cpu_LDL_C()      { LOAD(cpu_L, cpu_C); }
void cpu_LDL_D()      { LOAD(cpu_L, cpu_D); }
void cpu_LDL_E()      { LOAD(cpu_L, cpu_E); }
void cpu_LDL_H()      { LOAD(cpu_L, cpu_H); }
void cpu_LDL_L()      { LOAD(cpu_L, cpu_L); }
void cpu_LDL_AT_HL()  { LOAD(cpu_L, FETCH(cpu_H, cpu_L)); }
void cpu_LD_AT_HL_B() { STORE(cpu_H, cpu_L, cpu_B); }
void cpu_LD_AT_HL_C() { STORE(cpu_H, cpu_L, cpu_C); }
void cpu_LD_AT_HL_D() { STORE(cpu_H, cpu_L, cpu_D); }
void cpu_LD_AT_HL_E() { STORE(cpu_H, cpu_L, cpu_E); }
void cpu_LD_AT_HL_H() { STORE(cpu_H, cpu_L, cpu_H); }
void cpu_LD_AT_HL_L() { STORE(cpu_H, cpu_L, cpu_L); }
void cpu_LD_AT_HL_n() { STORE(cpu_H, cpu_L, mem_rb(cpu_PC++)); }
void cpu_LDA_AT_BC()  { LOAD(cpu_A, FETCH(cpu_B, cpu_C)); }
void cpu_LDA_AT_DE()  { LOAD(cpu_A, FETCH(cpu_D, cpu_E)); }
void cpu_LD_AT_BC_A() { STORE(cpu_B, cpu_C, cpu_A); }
void cpu_LD_AT_DE_A() { STORE(cpu_D, cpu_E, cpu_A); }
void cpu_LD_AT_HL_A() { STORE(cpu_H, cpu_L, cpu_A); }
void cpu_LDA_AT_C()   { LOAD(cpu_A, mem_rb(0xFF00 + cpu_C)); }
void cpu_LD_AT_C_A()  { STORE(0xFF, cpu_C, cpu_A); }

void cpu_LDA_AT_nn() {
   LOAD(cpu_A, mem_rb(mem_rw(cpu_PC)));
   cpu_PC += 2;
}

void cpu_LD_AT_nn_A() {
   mem_wb(mem_rw(cpu_PC), cpu_A);
   cpu_PC += 2;
}

void cpu_LDA_AT_HLD() {
   cpu_A = FETCH(cpu_H, cpu_L);
   cpu_DEC16(&cpu_H, &cpu_L);
}

void cpu_LDA_AT_HLI() {
   cpu_A = FETCH(cpu_H, cpu_L);
   cpu_INC16(&cpu_H, &cpu_L);
}

void cpu_LD_AT_HLD_A() {
   STORE(cpu_H, cpu_L, cpu_A);
   cpu_DEC16(&cpu_H, &cpu_L);
}

void cpu_LD_AT_HLI_A() {
   STORE(cpu_H, cpu_L, cpu_A);
   cpu_INC16(&cpu_H, &cpu_L);
}

void cpu_LD_n_A() {
   STORE(0xFF, mem_rb(cpu_PC++), cpu_A);
}

void cpu_LD_A_n() {
   cpu_A = FETCH(0xFF, mem_rb(cpu_PC++));
}

void cpu_LDBC_nn() {
   cpu_C = mem_rb(cpu_PC++);
   cpu_B = mem_rb(cpu_PC++);
}

void cpu_LDDE_nn() {
   cpu_E = mem_rb(cpu_PC++);
   cpu_D = mem_rb(cpu_PC++);
}

void cpu_LDHL_nn() {
   cpu_L = mem_rb(cpu_PC++);
   cpu_H = mem_rb(cpu_PC++);
}

void cpu_LDSP_nn() {
   cpu_SP = mem_rw(cpu_PC);
   cpu_PC += 2;
}

void cpu_LDSP_HL() {
   cpu_SP = (cpu_H << 8) | cpu_L;
   cpu_advance_time(8);
}

void cpu_LDHL_SP_n() {
   byte  next = mem_rb(cpu_PC++);
   sbyte off  = (sbyte)next;
   int   res  = off + cpu_SP;
   CLEAR_FLAGS();
   FLAG_C = ((cpu_SP & 0xFF) + next > 0xFF);
   FLAG_H = ((cpu_SP & 0xF) + (next & 0xF) > 0xF);
   cpu_H  = (res & 0xFF00) >> 8;
   cpu_L  = res & 0x00FF;
   cpu_advance_time(4444);
}

void cpu_LD_nn_SP() {
   word tmp = mem_rw(cpu_PC);
   cpu_PC += 2;
   mem_ww(tmp, cpu_SP);
}

// PUSH / POP

void cpu_PUSHBC() { PUSH(cpu_B, cpu_C); }
void cpu_PUSHDE() { PUSH(cpu_D, cpu_E); }
void cpu_PUSHHL() { PUSH(cpu_H, cpu_L); }
void cpu_POPBC() { POP(cpu_B, cpu_C); }
void cpu_POPDE() { POP(cpu_D, cpu_E); }
void cpu_POPHL() { POP(cpu_H, cpu_L); }

void cpu_PUSHAF() {
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
   PUSH(cpu_A, flags);
}

void cpu_POPAF() {
   byte flags = 0;
   POP(cpu_A, flags);
   FLAG_Z = (flags & BITMASK_Z);
   FLAG_C = (flags & BITMASK_C);
   FLAG_H = (flags & BITMASK_H);
   FLAG_N = (flags & BITMASK_N);
}

// AND / OR / XOR
void cpu_AND_A()     { AND(cpu_A); }
void cpu_AND_B()     { AND(cpu_B); }
void cpu_AND_C()     { AND(cpu_C); }
void cpu_AND_D()     { AND(cpu_D); }
void cpu_AND_E()     { AND(cpu_E); }
void cpu_AND_H()     { AND(cpu_H); }
void cpu_AND_L()     { AND(cpu_L); }
void cpu_AND_AT_HL() { AND(FETCH(cpu_H, cpu_L)); }
void cpu_AND_n()     { AND(mem_rb(cpu_PC++)); }
void cpu_OR_A()      { OR(cpu_A); }
void cpu_OR_B()      { OR(cpu_B); }
void cpu_OR_C()      { OR(cpu_C); }
void cpu_OR_D()      { OR(cpu_D); }
void cpu_OR_E()      { OR(cpu_E); }
void cpu_OR_H()      { OR(cpu_H); }
void cpu_OR_L()      { OR(cpu_L); }
void cpu_OR_AT_HL()  { OR(FETCH(cpu_H, cpu_L)); }
void cpu_OR_n()      { OR(mem_rb(cpu_PC++)); }
void cpu_XOR_A()     { XOR(cpu_A); }
void cpu_XOR_B()     { XOR(cpu_B); }
void cpu_XOR_C()     { XOR(cpu_C); }
void cpu_XOR_D()     { XOR(cpu_D); }
void cpu_XOR_E()     { XOR(cpu_E); }
void cpu_XOR_H()     { XOR(cpu_H); }
void cpu_XOR_L()     { XOR(cpu_L); }
void cpu_XOR_AT_HL() { XOR(FETCH(cpu_H, cpu_L)); }
void cpu_XOR_n()     { XOR(mem_rb(cpu_PC++)); }

void cpu_RLC(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   FLAG_C = (t & 0x80);
   t = (t << 1) | FLAG_C;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_RRC(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   FLAG_C = (t & 0x01);
   t = (t >> 1) | (FLAG_C << 7);
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_RL(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   byte old_carry = FLAG_C;
   FLAG_C = (t & 0x80);
   t = (t << 1) | old_carry;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_RR(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   byte old_carry = FLAG_C;
   FLAG_C = (t & 0x01);
   t = (t >> 1) | (old_carry << 7);
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_SLA(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   FLAG_C = (t & 0x80);
   t = t << 1;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_SRA(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   byte msb = t & 0x80;
   FLAG_C = (t & 0x01);
   t = (t >> 1) | msb;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_SWAP(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   t = ((t << 4) | (t >> 4));
   CLEAR_FLAGS();
   FLAG_Z = (t == 0);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_SRL(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   FLAG_C = (t & 0x01);
   t = t >> 1;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_BIT(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
      // Conflicting info here- Official GB doc
      // says 12 total cycles, pan docs say 16.
      // Trying 16.
      cpu_advance_time(4);
   }

   FLAG_Z = (!(t & (1 << bit)));
   FLAG_N = (false);
   FLAG_H = (true);
}

void cpu_RES(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   t = t & ~(1 << bit);

   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_SET(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_H, cpu_L);
   }

   t = t | (1 << bit);


   if (inp == NULL) {
      STORE(cpu_H, cpu_L, t);
   } else {
      *inp = t;
   }
}

void cpu_CB() {
   byte  sub_op = mem_rb(cpu_PC++);
   byte* regs[] = {&cpu_B, &cpu_C, &cpu_D, &cpu_E,
                   &cpu_H, &cpu_L, NULL,   &cpu_A};
   byte index = sub_op & 0x0F;
   if (index >= 8) {
      index -= 8;
      switch (sub_op & 0xF0) {
         case 0x00: cpu_RRC(regs[index]); break;
         case 0x10: cpu_RR(regs[index]); break;
         case 0x20: cpu_SRA(regs[index]); break;
         case 0x30: cpu_SRL(regs[index]); break;
         case 0x40: cpu_BIT(regs[index], 1); break;
         case 0x50: cpu_BIT(regs[index], 3); break;
         case 0x60: cpu_BIT(regs[index], 5); break;
         case 0x70: cpu_BIT(regs[index], 7); break;
         case 0x80: cpu_RES(regs[index], 1); break;
         case 0x90: cpu_RES(regs[index], 3); break;
         case 0xA0: cpu_RES(regs[index], 5); break;
         case 0xB0: cpu_RES(regs[index], 7); break;
         case 0xC0: cpu_SET(regs[index], 1); break;
         case 0xD0: cpu_SET(regs[index], 3); break;
         case 0xE0: cpu_SET(regs[index], 5); break;
         case 0xF0: cpu_SET(regs[index], 7); break;
         default: ERROR("CB opcode error"); break;
      }

   } else {
      switch (sub_op & 0xF0) {
         case 0x00: cpu_RLC(regs[index]); break;
         case 0x10: cpu_RL(regs[index]); break;
         case 0x20: cpu_SLA(regs[index]); break;
         case 0x30: cpu_SWAP(regs[index]); break;
         case 0x40: cpu_BIT(regs[index], 0); break;
         case 0x50: cpu_BIT(regs[index], 2); break;
         case 0x60: cpu_BIT(regs[index], 4); break;
         case 0x70: cpu_BIT(regs[index], 6); break;
         case 0x80: cpu_RES(regs[index], 0); break;
         case 0x90: cpu_RES(regs[index], 2); break;
         case 0xA0: cpu_RES(regs[index], 4); break;
         case 0xB0: cpu_RES(regs[index], 6); break;
         case 0xC0: cpu_SET(regs[index], 0); break;
         case 0xD0: cpu_SET(regs[index], 2); break;
         case 0xE0: cpu_SET(regs[index], 4); break;
         case 0xF0: cpu_SET(regs[index], 6); break;
         default: ERROR("CB opcode error"); break;
      }
   }
}

// JUMP / RETURN

void cpu_JP_nn() {
   JP();
}

void cpu_JP_NZ_nn() {
   if (!FLAG_Z) {
      JP();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_JP_Z_nn() {
   if (FLAG_Z) {
      JP();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_JP_NC_nn() {
   if (!FLAG_C) {
      JP();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_JP_C_nn() {
   if (FLAG_C) {
      JP();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_JP_AT_HL() {
   cpu_PC = ((cpu_H << 8) | cpu_L);
}

void cpu_JR_n() {
   JR();
}

void cpu_JR_NZ_n() {
   if (!FLAG_Z) {
      JR();
   } else {
      cpu_advance_time(4);
      cpu_PC++;
   }
}

void cpu_JR_Z_n() {
   if (FLAG_Z) {
      JR();
   } else {
      cpu_advance_time(4);
      cpu_PC++;
   }
}

void cpu_JR_NC_n() {
   if (!FLAG_C) {
      JR();
   } else {
      cpu_advance_time(4);
      cpu_PC++;
   }
}

void cpu_JR_C_n() {
   if (FLAG_C) {
      JR();
   } else {
      cpu_advance_time(4);
      cpu_PC++;
   }
}

void cpu_CALL() {
   PUSHW(cpu_PC + 2);
   cpu_PC = mem_rw(cpu_PC);
   cpu_advance_time(4);
}

void cpu_CALL_nn() {
   cpu_CALL();
}

void cpu_CALL_NZ_nn() {
   if (!FLAG_Z) {
      cpu_CALL();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_CALL_Z_nn() {
   if (FLAG_Z) {
      cpu_CALL();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_CALL_NC_nn() {
   if (!FLAG_C) {
      cpu_CALL();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

void cpu_CALL_C_nn() {
   if (FLAG_C) {
      cpu_CALL();
   } else {
      cpu_advance_time(8);
      cpu_PC += 2;
   }
}

// Restarts

void cpu_RST_00H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x00;
   cpu_advance_time(4);
}

void cpu_RST_08H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x08;
   cpu_advance_time(4);
}

void cpu_RST_10H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x10;
   cpu_advance_time(4);
}

void cpu_RST_18H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x18;
   cpu_advance_time(4);
}

void cpu_RST_20H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x20;
   cpu_advance_time(4);
}

void cpu_RST_28H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x28;
   cpu_advance_time(4);
}

void cpu_RST_30H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x30;
   cpu_advance_time(4);
}

void cpu_RST_38H() {
   PUSHW(cpu_PC);
   cpu_halt = false;
   cpu_stop = false;
   cpu_PC   = 0x38;
   cpu_advance_time(4);

   // If reset 38 is pointing to itself, error out
   if (mem_direct_read(cpu_PC) == 0xFF) {
      ERROR("Reset 38H loop detected.\n");
   }
}

// Returns

void cpu_RETURN() {
   POPW(cpu_PC);
   cpu_advance_time(4);
}

void cpu_RET() {
   cpu_RETURN();
}

void cpu_RET_NZ() {
   if (!FLAG_Z) {
      cpu_RETURN();
   } else {
      cpu_advance_time(4);
   }
}

void cpu_RET_Z() {
   if (FLAG_Z) {
      cpu_RETURN();
   } else {
      cpu_advance_time(4);
   }
}

void cpu_RET_NC() {
   if (!FLAG_C) {
      cpu_RETURN();
   } else {
      cpu_advance_time(4);
   }
}

void cpu_RET_C() {
   if (FLAG_C) {
      cpu_RETURN();
   } else {
      cpu_advance_time(4);
   }
}

void cpu_RETI() {
   cpu_ei = true;
   cpu_ei_delay = true; // TODO: I don't know if RETI should delay
   DEBUG("INTERRUPTS ENABLED\n");
   cpu_RETURN();
}

// 16 bit math

void cpu_ADD16(byte* a_hi, byte* a_low, byte b_hi, byte b_low) {
   word a = (*a_hi << 8) | *a_low;
   word b = (b_hi << 8) | b_low;

   uint32_t carryCheck = a + b;
   FLAG_H = ((0x0FFF & a) + (0x0FFF & b) > 0x0FFF);
   FLAG_C = (carryCheck > 0x0000FFFF);
   FLAG_N = (false);

   a += b;
   *a_hi  = (a & 0xFF00) >> 8;
   *a_low = a & 0x00FF;
   cpu_advance_time(4);
}

void cpu_INC16(byte* hi, byte* low) {
   if ((*low) == 0xFF) {
      (*hi)++;
   }
   (*low)++;
}

void cpu_DEC16(byte* hi, byte* low) {
   if ((*low) == 0) {
      (*hi)--;
   }
   (*low)--;
}

void cpu_ADD16_HL_BC() {
   cpu_ADD16(&cpu_H, &cpu_L, cpu_B, cpu_C);
}

void cpu_ADD16_HL_DE() {
   cpu_ADD16(&cpu_H, &cpu_L, cpu_D, cpu_E);
}

void cpu_ADD16_HL_HL() {
   cpu_ADD16(&cpu_H, &cpu_L, cpu_H, cpu_L);
}

void cpu_ADD16_HL_SP() {
   cpu_ADD16(&cpu_H, &cpu_L, (cpu_SP & 0xFF00) >> 8, cpu_SP & 0x00FF);
}

void cpu_ADD16_SP_n() {
   byte  val = mem_rb(cpu_PC++);
   sbyte off = (sbyte)val;
   CLEAR_FLAGS();
   FLAG_H = ((cpu_SP & 0xF) + (val & 0xF) > 0xF);
   FLAG_C = (((cpu_SP & 0xFF) + val > 0xFF));
   cpu_SP += off;
   cpu_advance_time(8);
}

// 0x03
void cpu_INC16_BC() {
   cpu_INC16(&cpu_B, &cpu_C);
   cpu_advance_time(4);
}

void cpu_INC16_DE() {
   cpu_INC16(&cpu_D, &cpu_E);
   cpu_advance_time(4);
}

void cpu_INC16_HL() {
   cpu_INC16(&cpu_H, &cpu_L);
   cpu_advance_time(4);
}

void cpu_INC16_SP() {
   cpu_SP++;
   cpu_advance_time(4);
}

void cpu_DEC16_BC() {
   cpu_DEC16(&cpu_B, &cpu_C);
}

void cpu_DEC16_DE() {
   cpu_DEC16(&cpu_D, &cpu_E);
}

void cpu_DEC16_HL() {
   cpu_DEC16(&cpu_H, &cpu_L);
}

void cpu_DEC16_SP() {
   cpu_SP--;
   cpu_advance_time(4);
}

// Rotates / misc

void cpu_RLA() {
   cpu_RL(&cpu_A);
   FLAG_Z = false;
}

void cpu_RLCA() {
   cpu_RLC(&cpu_A);
   FLAG_Z = false;
}

void cpu_RRCA() {
   cpu_RRC(&cpu_A);
   FLAG_Z = false;
}

void cpu_RRA() {
   cpu_RR(&cpu_A);
   FLAG_Z = false;
}

void cpu_DI() {
   cpu_ei = false;
   cpu_ei_delay = false;
   DEBUG("INTERRUPTS DISABLED\n");
}

void cpu_EI() {
   DEBUG("INTERRUPTS ENABLED\n");
   cpu_ei       = true;
   cpu_ei_delay = true;
}

// ADD / ADC / SUB / SUBC
void cpu_ADD_A_A()     { ADD(cpu_A); }
void cpu_ADD_A_B()     { ADD(cpu_B); }
void cpu_ADD_A_C()     { ADD(cpu_C); }
void cpu_ADD_A_D()     { ADD(cpu_D); }
void cpu_ADD_A_E()     { ADD(cpu_E); }
void cpu_ADD_A_H()     { ADD(cpu_H); }
void cpu_ADD_A_L()     { ADD(cpu_L); }
void cpu_ADD_A_AT_HL() { ADD(FETCH(cpu_H, cpu_L)); }
void cpu_ADD_A_n()     { ADD(mem_rb(cpu_PC++)); }
void cpu_ADC_A_A()     { ADC(cpu_A); }
void cpu_ADC_A_B()     { ADC(cpu_B); }
void cpu_ADC_A_C()     { ADC(cpu_C); }
void cpu_ADC_A_D()     { ADC(cpu_D); }
void cpu_ADC_A_E()     { ADC(cpu_E); }
void cpu_ADC_A_H()     { ADC(cpu_H); }
void cpu_ADC_A_L()     { ADC(cpu_L); }
void cpu_ADC_A_AT_HL() { ADC(FETCH(cpu_H, cpu_L)); }
void cpu_ADC_A_n()     { ADC(mem_rb(cpu_PC++)); }
void cpu_SUB_A_A()     { SUB(cpu_A); }
void cpu_SUB_A_B()     { SUB(cpu_B); }
void cpu_SUB_A_C()     { SUB(cpu_C); }
void cpu_SUB_A_D()     { SUB(cpu_D); }
void cpu_SUB_A_E()     { SUB(cpu_E); }
void cpu_SUB_A_H()     { SUB(cpu_H); }
void cpu_SUB_A_L()     { SUB(cpu_L); }
void cpu_SUB_A_AT_HL() { SUB(FETCH(cpu_H, cpu_L)); }
void cpu_SUB_A_n()     { SUB(mem_rb(cpu_PC++)); }
void cpu_SBC_A_A()     { SBC(cpu_A); }
void cpu_SBC_A_B()     { SBC(cpu_B); }
void cpu_SBC_A_C()     { SBC(cpu_C); }
void cpu_SBC_A_D()     { SBC(cpu_D); }
void cpu_SBC_A_E()     { SBC(cpu_E); }
void cpu_SBC_A_H()     { SBC(cpu_H); }
void cpu_SBC_A_L()     { SBC(cpu_L); }
void cpu_SBC_A_AT_HL() { SBC(FETCH(cpu_H, cpu_L)); }
void cpu_SBC_A_n()     { SBC(mem_rb(cpu_PC++)); }

// INCREMENT / DECREMENT
void cpu_INC_A() { INC(cpu_A); }
void cpu_INC_B() { INC(cpu_B); }
void cpu_INC_C() { INC(cpu_C); }
void cpu_INC_D() { INC(cpu_D); }
void cpu_INC_E() { INC(cpu_E); }
void cpu_INC_H() { INC(cpu_H); }
void cpu_INC_L() { INC(cpu_L); }
void cpu_DEC_A() { DEC(cpu_A); }
void cpu_DEC_B() { DEC(cpu_B); }
void cpu_DEC_C() { DEC(cpu_C); }
void cpu_DEC_D() { DEC(cpu_D); }
void cpu_DEC_E() { DEC(cpu_E); }
void cpu_DEC_H() { DEC(cpu_H); }
void cpu_DEC_L() { DEC(cpu_L); }

void cpu_INC_AT_HL() {
   byte val = FETCH(cpu_H, cpu_L);
   FLAG_H = (val & 0x0F) + 1 > 0x0F; 
   FLAG_N = false; 
   val++; 
   FLAG_Z = val == 0; 
   STORE(cpu_H, cpu_L, val);
}

void cpu_DEC_AT_HL() {
   byte val = FETCH(cpu_H, cpu_L);
   FLAG_H = (val & 0x0F) == 0; 
   val--; 
   FLAG_Z = val == 0; 
   FLAG_N = true; 
   STORE(cpu_H, cpu_L, val);
}

// COMPARE
void cpu_CP_A()      { COMPARE(cpu_A); }
void cpu_CP_B()      { COMPARE(cpu_B); }
void cpu_CP_C()      { COMPARE(cpu_C); }
void cpu_CP_D()      { COMPARE(cpu_D); }
void cpu_CP_E()      { COMPARE(cpu_E); }
void cpu_CP_H()      { COMPARE(cpu_H); }
void cpu_CP_L()      { COMPARE(cpu_L); }
void cpu_CP_AT_HL()  { COMPARE(FETCH(cpu_H, cpu_L)); }
void cpu_CP_A_n()    { COMPARE(mem_rb(cpu_PC++)); }

void cpu_CPL() {
   cpu_A = ~cpu_A;
   FLAG_H = true;
   FLAG_N = true;
}

void cpu_CCF() {
   FLAG_C = !FLAG_C;
   FLAG_N = false;
   FLAG_H = false;
}

void cpu_SCF() {
   FLAG_H = false;
   FLAG_N = false;
   FLAG_C = true;
}

void cpu_HALT() {
      cpu_halt = true;
      DEBUG("HALT\n");
}

void cpu_STOP() {
   cpu_stop = true;
   DEBUG("STOP\n");
}
/*
void cpu_DAA() {
   word a = cpu_A;
   if (!FLAG_N) {
      if (FLAG_H) {
         if (FLAG_C) {
            a += 0x100;
         }
         CLEAR_FLAGS();
         a += 0x06;
         if (a >= 0xA0) {
            a -= 0xA0;
            FLAG_C = true;
         }
      } else {
         if (FLAG_C) {
            a += 0x100;
         }
         if (a > 0x99) {
            a += 0x60;
         }
         a = (a & 0xF)
           + (a & 0xF) > 9
                 ? 6
                 : 0
           + (a & 0xFF0);
         FLAG_C = a > 0xFF;
      }
  } else {
      if (FLAG_H) {
         if (FLAG_C) {
            a += 0x9A;
         } else {
            a += 0xFA;
         }
      } else if (FLAG_C) {
         a += 0xA0;
      }
   }

   cpu_A = 0xFF & a;
   FLAG_H = false;
   FLAG_Z = cpu_A == 0;
}
*/
void cpu_DAA() {
   int a = cpu_A;
   if (!FLAG_N) {
      if (FLAG_H || (a & 0xF) > 9) {
         a += 0x06;
      }
      if (FLAG_C || a > 0x9F) {
         a += 0x60;
      }
   } else {
      if (FLAG_H) {
         a = (a - 6) & 0xFF;
      }
      if (FLAG_C) {
         a -= 0x60;
      }
   }

   FLAG_H = false;
   FLAG_C = (a & 0x100) == 100;
   a = a & 0xFF;
   FLAG_Z = a == 0;
   cpu_A = (byte)a;
}
