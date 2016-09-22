#include "z80.h"
#include "gpu.h"

// Opcode macros

#define LOAD(reg, val) \
   reg = (val); 

// rb and wb advance time by 4 ticks each
#define STORE(hi, lo, val) \
   mem_wb((((hi) & 0xFF) << 8) | ((lo) & 0xFF), (val)); 

#define FETCH(hi, lo) \
   mem_rb((((hi) & 0xFF) << 8) | ((lo) & 0xFF))

#define AND(val) \
   z80_A = z80_A & (val); \
   FLAG_C = FLAG_N = false; \
   FLAG_Z = !z80_A; \
   FLAG_H = true; 

#define OR(val) \
   z80_A = z80_A | (val); \
   FLAG_C = FLAG_H = FLAG_N = false; \
   FLAG_Z = !z80_A; 

#define XOR(val) \
   z80_A = z80_A ^ (val); \
   FLAG_C = FLAG_H = FLAG_N = false; \
   FLAG_Z = !z80_A; 

#define PUSH(hi, lo) \
   mem_direct_write(--z80_SP, (hi)); \
   mem_direct_write(--z80_SP, (lo)); \
   z80_advance_time(12);

#define POP(hi, lo) \
   lo = mem_direct_read(z80_SP++); \
   hi = mem_direct_read(z80_SP++); \
   z80_advance_time(8);

#define PUSHW(val) \
   z80_SP -= 2; \
   mem_ww(z80_SP, (val)); 

#define POPW(val) \
   val = mem_rw(z80_SP); \
   z80_SP += 2; \

#define JP() \
   z80_PC = mem_rw(z80_PC); \
   z80_advance_time(4);

#define JR() \
   z80_PC += (sbyte)mem_rb(z80_PC) + 1; \
   z80_advance_time(4);

#define ADD(val) \
   byte v = (val); \
   FLAG_N = false; \
   FLAG_H = ((z80_A & 0x0F) + (v & 0x0F)) > 0x0F; \
   word tmp = z80_A + v; \
   z80_A = tmp & 0xFF; \
   FLAG_C = tmp > 0xFF; \
   FLAG_Z = z80_A == 0; 

#define ADC(val) \
   byte v = (val); \
   FLAG_N = false; \
   word tmp = z80_A + v + FLAG_C; \
   FLAG_H = (z80_A & 0x0F) + (v & 0x0F) + FLAG_C > 0x0F; \
   FLAG_C = tmp > 0xFF; \
   z80_A = tmp & 0xFF; \
   FLAG_Z = z80_A == 0; 

#define SUB(val) \
   byte v = (val); \
   FLAG_N = true; \
   FLAG_H = (z80_A & 0x0F) < (v & 0x0F); \
   FLAG_C = z80_A < v; \
   z80_A -= v; \
   FLAG_Z = z80_A == 0; 

#define SBC(val) \
   byte v = (val); \
   FLAG_N = true; \
   byte tmp = z80_A - v - FLAG_C; \
   FLAG_H = (z80_A & 0x0F) < (v & 0x0F) + FLAG_C; \
   FLAG_C = z80_A < v + FLAG_C; \
   z80_A = tmp; \
   FLAG_Z = z80_A == 0; 

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
   FLAG_H = (z80_A & 0x0F) < (v & 0x0F); \
   FLAG_C = z80_A < v; \
   FLAG_N = true; \
   FLAG_Z = z80_A == v; 

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

void z80_init(char* romname) {
   z80_ei         = false;
   z80_ei_delay   = 0;
   z80_tima_timer = 0;
   z80_div_timer  = 0;
   z80_halt       = false;
   z80_stop       = false;
   z80_rom_fname  = romname;

   for (size_t i = 0; i < 0x100; i++) {
      z80_opcodes[i] = &z80_NI;
   }

                                           // Timing

   z80_opcodes[0x00] = &z80_NOP;           /* 1 */ 
   z80_opcodes[0x01] = &z80_LDBC_nn;       /* 3 */
   z80_opcodes[0x02] = &z80_LD_AT_BC_A;    /* 2 */
   z80_opcodes[0x03] = &z80_INC16_BC;      /* 2 */
   z80_opcodes[0x04] = &z80_INC_B;         /* 1 */
   z80_opcodes[0x05] = &z80_DEC_B;         /* 1 */
   z80_opcodes[0x06] = &z80_LDB_n;         /* 2 */
   z80_opcodes[0x07] = &z80_RLCA;          /* 1 */
   z80_opcodes[0x08] = &z80_LD_nn_SP;      /* 5 */
   z80_opcodes[0x09] = &z80_ADD16_HL_BC;   /* 2 */
   z80_opcodes[0x0A] = &z80_LDA_AT_BC;     /* 2 */
   z80_opcodes[0x0B] = &z80_DEC16_BC;      /* 2 */
   z80_opcodes[0x0C] = &z80_INC_C;         /* 1 */
   z80_opcodes[0x0D] = &z80_DEC_C;         /* 1 */
   z80_opcodes[0x0E] = &z80_LDC_n;         /* 2 */
   z80_opcodes[0x0F] = &z80_RRCA;          /* 1 */
                                           
   z80_opcodes[0x10] = &z80_STOP;          /* 0 */
   z80_opcodes[0x11] = &z80_LDDE_nn;       /* 3 */
   z80_opcodes[0x12] = &z80_LD_AT_DE_A;    /* 2 */
   z80_opcodes[0x13] = &z80_INC16_DE;      /* 2 */
   z80_opcodes[0x14] = &z80_INC_D;         /* 1 */
   z80_opcodes[0x15] = &z80_DEC_D;         /* 1 */
   z80_opcodes[0x16] = &z80_LDD_n;         /* 2 */
   z80_opcodes[0x17] = &z80_RLA;           /* 1 */
   z80_opcodes[0x18] = &z80_JR_n;          /* 3 */
   z80_opcodes[0x19] = &z80_ADD16_HL_DE;   /* 2 */
   z80_opcodes[0x1A] = &z80_LDA_AT_DE;     /* 2 */
   z80_opcodes[0x1B] = &z80_DEC16_DE;      /* 2 */
   z80_opcodes[0x1C] = &z80_INC_E;         /* 1 */
   z80_opcodes[0x1D] = &z80_DEC_E;         /* 1 */
   z80_opcodes[0x1E] = &z80_LDE_n;         /* 2 */
   z80_opcodes[0x1F] = &z80_RRA;           /* 1 */
                                           
   z80_opcodes[0x20] = &z80_JR_NZ_n;       /* 2 */
   z80_opcodes[0x21] = &z80_LDHL_nn;       /* 3 */
   z80_opcodes[0x22] = &z80_LD_AT_HLI_A;   /* 2 */
   z80_opcodes[0x23] = &z80_INC16_HL;      /* 2 */
   z80_opcodes[0x24] = &z80_INC_H;         /* 1 */
   z80_opcodes[0x25] = &z80_DEC_H;         /* 1 */
   z80_opcodes[0x26] = &z80_LDH_n;         /* 2 */
   z80_opcodes[0x27] = &z80_DAA;           /* 1 */
   z80_opcodes[0x28] = &z80_JR_Z_n;        /* 2 */
   z80_opcodes[0x29] = &z80_ADD16_HL_HL;   /* 2 */
   z80_opcodes[0x2A] = &z80_LDA_AT_HLI;    /* 2 */
   z80_opcodes[0x2B] = &z80_DEC16_HL;      /* 2 */
   z80_opcodes[0x2C] = &z80_INC_L;         /* 1 */
   z80_opcodes[0x2D] = &z80_DEC_L;         /* 1 */
   z80_opcodes[0x2E] = &z80_LDL_n;         /* 2 */
   z80_opcodes[0x2F] = &z80_CPL;           /* 1 */
                                           
   z80_opcodes[0x30] = &z80_JR_NC_n;       /* 2 */
   z80_opcodes[0x31] = &z80_LDSP_nn;       /* 3 */
   z80_opcodes[0x32] = &z80_LD_AT_HLD_A;   /* 2 */
   z80_opcodes[0x33] = &z80_INC16_SP;      /* 2 */
   z80_opcodes[0x34] = &z80_INC_AT_HL;     /* 3 */
   z80_opcodes[0x35] = &z80_DEC_AT_HL;     /* 3 */
   z80_opcodes[0x36] = &z80_LD_AT_HL_n;    /* 3 */
   z80_opcodes[0x37] = &z80_SCF;           /* 1 */
   z80_opcodes[0x38] = &z80_JR_C_n;        /* 2 */
   z80_opcodes[0x39] = &z80_ADD16_HL_SP;   /* 2 */
   z80_opcodes[0x3A] = &z80_LDA_AT_HLD;    /* 2 */
   z80_opcodes[0x3B] = &z80_DEC16_SP;      /* 2 */
   z80_opcodes[0x3C] = &z80_INC_A;         /* 1 */
   z80_opcodes[0x3D] = &z80_DEC_A;         /* 1 */
   z80_opcodes[0x3E] = &z80_LDA_n;         /* 2 */
   z80_opcodes[0x3F] = &z80_CCF;           /* 1 */
                                           
   z80_opcodes[0x40] = &z80_LDB_B;         /* 1 */
   z80_opcodes[0x41] = &z80_LDB_C;         /* 1 */
   z80_opcodes[0x42] = &z80_LDB_D;         /* 1 */
   z80_opcodes[0x43] = &z80_LDB_E;         /* 1 */
   z80_opcodes[0x44] = &z80_LDB_H;         /* 1 */
   z80_opcodes[0x45] = &z80_LDB_L;         /* 1 */
   z80_opcodes[0x46] = &z80_LDB_AT_HL;     /* 2 */
   z80_opcodes[0x47] = &z80_LDB_A;         /* 1 */
   z80_opcodes[0x48] = &z80_LDC_B;         /* 1 */
   z80_opcodes[0x49] = &z80_LDC_C;         /* 1 */
   z80_opcodes[0x4A] = &z80_LDC_D;         /* 1 */
   z80_opcodes[0x4B] = &z80_LDC_E;         /* 1 */
   z80_opcodes[0x4C] = &z80_LDC_H;         /* 1 */
   z80_opcodes[0x4D] = &z80_LDC_L;         /* 1 */
   z80_opcodes[0x4E] = &z80_LDC_AT_HL;     /* 2 */
   z80_opcodes[0x4F] = &z80_LDC_A;         /* 1 */
                                           
   z80_opcodes[0x50] = &z80_LDD_B;         /* 1 */
   z80_opcodes[0x51] = &z80_LDD_C;         /* 1 */
   z80_opcodes[0x52] = &z80_LDD_D;         /* 1 */
   z80_opcodes[0x53] = &z80_LDD_E;         /* 1 */
   z80_opcodes[0x54] = &z80_LDD_H;         /* 1 */
   z80_opcodes[0x55] = &z80_LDD_L;         /* 1 */
   z80_opcodes[0x56] = &z80_LDD_AT_HL;     /* 2 */
   z80_opcodes[0x57] = &z80_LDD_A;         /* 1 */
   z80_opcodes[0x58] = &z80_LDE_B;         /* 1 */
   z80_opcodes[0x59] = &z80_LDE_C;         /* 1 */
   z80_opcodes[0x5A] = &z80_LDE_D;         /* 1 */
   z80_opcodes[0x5B] = &z80_LDE_E;         /* 1 */
   z80_opcodes[0x5C] = &z80_LDE_H;         /* 1 */
   z80_opcodes[0x5D] = &z80_LDE_L;         /* 1 */
   z80_opcodes[0x5E] = &z80_LDE_AT_HL;     /* 2 */
   z80_opcodes[0x5F] = &z80_LDE_A;         /* 1 */
                                           
   z80_opcodes[0x60] = &z80_LDH_B;         /* 1 */
   z80_opcodes[0x61] = &z80_LDH_C;         /* 1 */
   z80_opcodes[0x62] = &z80_LDH_D;         /* 1 */
   z80_opcodes[0x63] = &z80_LDH_E;         /* 1 */
   z80_opcodes[0x64] = &z80_LDH_H;         /* 1 */
   z80_opcodes[0x65] = &z80_LDH_L;         /* 1 */
   z80_opcodes[0x66] = &z80_LDH_AT_HL;     /* 2 */
   z80_opcodes[0x67] = &z80_LDH_A;         /* 1 */
   z80_opcodes[0x68] = &z80_LDL_B;         /* 1 */
   z80_opcodes[0x69] = &z80_LDL_C;         /* 1 */
   z80_opcodes[0x6A] = &z80_LDL_D;         /* 1 */
   z80_opcodes[0x6B] = &z80_LDL_E;         /* 1 */
   z80_opcodes[0x6C] = &z80_LDL_H;         /* 1 */
   z80_opcodes[0x6D] = &z80_LDL_L;         /* 1 */
   z80_opcodes[0x6E] = &z80_LDL_AT_HL;     /* 2 */
   z80_opcodes[0x6F] = &z80_LDL_A;         /* 1 */
                                           
   z80_opcodes[0x70] = &z80_LD_AT_HL_B;    /* 2 */
   z80_opcodes[0x71] = &z80_LD_AT_HL_C;    /* 2 */
   z80_opcodes[0x72] = &z80_LD_AT_HL_D;    /* 2 */
   z80_opcodes[0x73] = &z80_LD_AT_HL_E;    /* 2 */
   z80_opcodes[0x74] = &z80_LD_AT_HL_H;    /* 2 */
   z80_opcodes[0x75] = &z80_LD_AT_HL_L;    /* 2 */
   z80_opcodes[0x76] = &z80_HALT;          /* 0 */
   z80_opcodes[0x77] = &z80_LD_AT_HL_A;    /* 2 */
   z80_opcodes[0x78] = &z80_LDA_B;         /* 1 */
   z80_opcodes[0x79] = &z80_LDA_C;         /* 1 */
   z80_opcodes[0x7A] = &z80_LDA_D;         /* 1 */
   z80_opcodes[0x7B] = &z80_LDA_E;         /* 1 */
   z80_opcodes[0x7C] = &z80_LDA_H;         /* 1 */
   z80_opcodes[0x7D] = &z80_LDA_L;         /* 1 */
   z80_opcodes[0x7E] = &z80_LDA_AT_HL;     /* 2 */
   z80_opcodes[0x7F] = &z80_LDA_A;         /* 1 */
                                           
   z80_opcodes[0x80] = &z80_ADD_A_B;       /* 1 */
   z80_opcodes[0x81] = &z80_ADD_A_C;       /* 1 */
   z80_opcodes[0x82] = &z80_ADD_A_D;       /* 1 */
   z80_opcodes[0x83] = &z80_ADD_A_E;       /* 1 */
   z80_opcodes[0x84] = &z80_ADD_A_H;       /* 1 */
   z80_opcodes[0x85] = &z80_ADD_A_L;       /* 1 */
   z80_opcodes[0x86] = &z80_ADD_A_AT_HL;   /* 2 */
   z80_opcodes[0x87] = &z80_ADD_A_A;       /* 1 */
   z80_opcodes[0x88] = &z80_ADC_A_B;       /* 1 */
   z80_opcodes[0x89] = &z80_ADC_A_C;       /* 1 */
   z80_opcodes[0x8A] = &z80_ADC_A_D;       /* 1 */
   z80_opcodes[0x8B] = &z80_ADC_A_E;       /* 1 */
   z80_opcodes[0x8C] = &z80_ADC_A_H;       /* 1 */
   z80_opcodes[0x8D] = &z80_ADC_A_L;       /* 1 */
   z80_opcodes[0x8E] = &z80_ADC_A_AT_HL;   /* 2 */
   z80_opcodes[0x8F] = &z80_ADC_A_A;       /* 1 */
                                           
   z80_opcodes[0x90] = &z80_SUB_A_B;       /* 1 */
   z80_opcodes[0x91] = &z80_SUB_A_C;       /* 1 */
   z80_opcodes[0x92] = &z80_SUB_A_D;       /* 1 */
   z80_opcodes[0x93] = &z80_SUB_A_E;       /* 1 */
   z80_opcodes[0x94] = &z80_SUB_A_H;       /* 1 */
   z80_opcodes[0x95] = &z80_SUB_A_L;       /* 1 */
   z80_opcodes[0x96] = &z80_SUB_A_AT_HL;   /* 2 */
   z80_opcodes[0x97] = &z80_SUB_A_A;       /* 1 */
   z80_opcodes[0x98] = &z80_SBC_A_B;       /* 1 */
   z80_opcodes[0x99] = &z80_SBC_A_C;       /* 1 */
   z80_opcodes[0x9A] = &z80_SBC_A_D;       /* 1 */
   z80_opcodes[0x9B] = &z80_SBC_A_E;       /* 1 */
   z80_opcodes[0x9C] = &z80_SBC_A_H;       /* 1 */
   z80_opcodes[0x9D] = &z80_SBC_A_L;       /* 1 */
   z80_opcodes[0x9E] = &z80_SBC_A_AT_HL;   /* 2 */
   z80_opcodes[0x9F] = &z80_SBC_A_A;       /* 1 */
                                           
   z80_opcodes[0xA0] = &z80_AND_B;         /* 1 */
   z80_opcodes[0xA1] = &z80_AND_C;         /* 1 */
   z80_opcodes[0xA2] = &z80_AND_D;         /* 1 */
   z80_opcodes[0xA3] = &z80_AND_E;         /* 1 */
   z80_opcodes[0xA4] = &z80_AND_H;         /* 1 */
   z80_opcodes[0xA5] = &z80_AND_L;         /* 1 */
   z80_opcodes[0xA6] = &z80_AND_AT_HL;     /* 2 */
   z80_opcodes[0xA7] = &z80_AND_A;         /* 1 */
   z80_opcodes[0xA8] = &z80_XOR_B;         /* 1 */
   z80_opcodes[0xA9] = &z80_XOR_C;         /* 1 */
   z80_opcodes[0xAA] = &z80_XOR_D;         /* 1 */
   z80_opcodes[0xAB] = &z80_XOR_E;         /* 1 */
   z80_opcodes[0xAC] = &z80_XOR_H;         /* 1 */
   z80_opcodes[0xAD] = &z80_XOR_L;         /* 1 */
   z80_opcodes[0xAE] = &z80_XOR_AT_HL;     /* 2 */
   z80_opcodes[0xAF] = &z80_XOR_A;         /* 1 */
                                           
   z80_opcodes[0xB0] = &z80_OR_B;          /* 1 */
   z80_opcodes[0xB1] = &z80_OR_C;          /* 1 */
   z80_opcodes[0xB2] = &z80_OR_D;          /* 1 */
   z80_opcodes[0xB3] = &z80_OR_E;          /* 1 */
   z80_opcodes[0xB4] = &z80_OR_H;          /* 1 */
   z80_opcodes[0xB5] = &z80_OR_L;          /* 1 */
   z80_opcodes[0xB6] = &z80_OR_AT_HL;      /* 2 */
   z80_opcodes[0xB7] = &z80_OR_A;          /* 1 */
   z80_opcodes[0xB8] = &z80_CP_B;          /* 1 */
   z80_opcodes[0xB9] = &z80_CP_C;          /* 1 */
   z80_opcodes[0xBA] = &z80_CP_D;          /* 1 */
   z80_opcodes[0xBB] = &z80_CP_E;          /* 1 */
   z80_opcodes[0xBC] = &z80_CP_H;          /* 1 */
   z80_opcodes[0xBD] = &z80_CP_L;          /* 1 */
   z80_opcodes[0xBE] = &z80_CP_AT_HL;      /* 2 */
   z80_opcodes[0xBF] = &z80_CP_A;          /* 1 */
                                           
   z80_opcodes[0xC0] = &z80_RET_NZ;        /* 2 */
   z80_opcodes[0xC1] = &z80_POPBC;         /* 3 */
   z80_opcodes[0xC2] = &z80_JP_NZ_nn;      /* 3 */
   z80_opcodes[0xC3] = &z80_JP_nn;         /* 4 */
   z80_opcodes[0xC4] = &z80_CALL_NZ_nn;    /* 3 */
   z80_opcodes[0xC5] = &z80_PUSHBC;        /* 4 */
   z80_opcodes[0xC6] = &z80_ADD_A_n;       /* 2 */
   z80_opcodes[0xC7] = &z80_RST_00H;       /* 4 */
   z80_opcodes[0xC8] = &z80_RET_Z;         /* 2 */
   z80_opcodes[0xC9] = &z80_RET;           /* 4 */
   z80_opcodes[0xCA] = &z80_JP_Z_nn;       /* 3 */
   z80_opcodes[0xCB] = &z80_CB;            /* 0 */
   z80_opcodes[0xCC] = &z80_CALL_Z_nn;     /* 3 */
   z80_opcodes[0xCD] = &z80_CALL_nn;       /* 6 */
   z80_opcodes[0xCE] = &z80_ADC_A_n;       /* 2 */
   z80_opcodes[0xCF] = &z80_RST_08H;       /* 4 */
                                           
   z80_opcodes[0xD0] = &z80_RET_NC;        /* 2 */
   z80_opcodes[0xD1] = &z80_POPDE;         /* 3 */
   z80_opcodes[0xD2] = &z80_JP_NC_nn;      /* 3 */
   z80_opcodes[0xD3] = &z80_NI;            /* 0 */
   z80_opcodes[0xD4] = &z80_CALL_NC_nn;    /* 3 */
   z80_opcodes[0xD5] = &z80_PUSHDE;        /* 4 */
   z80_opcodes[0xD6] = &z80_SUB_A_n;       /* 2 */
   z80_opcodes[0xD7] = &z80_RST_10H;       /* 4 */
   z80_opcodes[0xD8] = &z80_RET_C;         /* 2 */
   z80_opcodes[0xD9] = &z80_RETI;          /* 4 */
   z80_opcodes[0xDA] = &z80_JP_C_nn;       /* 3 */
   z80_opcodes[0xDB] = &z80_NI;            /* 0 */
   z80_opcodes[0xDC] = &z80_CALL_C_nn;     /* 3 */
   z80_opcodes[0xDD] = &z80_NI;            /* 0 */
   z80_opcodes[0xDE] = &z80_SBC_A_n;       /* 2 */
   z80_opcodes[0xDF] = &z80_RST_18H;       /* 4 */
                                           
   z80_opcodes[0xE0] = &z80_LD_n_A;        /* 3 */
   z80_opcodes[0xE1] = &z80_POPHL;         /* 3 */
   z80_opcodes[0xE2] = &z80_LD_AT_C_A;     /* 2 */
   z80_opcodes[0xE3] = &z80_NI;            /* 0 */
   z80_opcodes[0xE4] = &z80_NI;            /* 0 */
   z80_opcodes[0xE5] = &z80_PUSHHL;        /* 4 */
   z80_opcodes[0xE6] = &z80_AND_n;         /* 2 */
   z80_opcodes[0xE7] = &z80_RST_20H;       /* 4 */
   z80_opcodes[0xE8] = &z80_ADD16_SP_n;    /* 4 */
   z80_opcodes[0xE9] = &z80_JP_AT_HL;      /* 1 */
   z80_opcodes[0xEA] = &z80_LD_AT_nn_A;    /* 4 */
   z80_opcodes[0xEB] = &z80_NI;            /* 0 */
   z80_opcodes[0xEC] = &z80_NI;            /* 0 */
   z80_opcodes[0xED] = &z80_NI;            /* 0 */
   z80_opcodes[0xEE] = &z80_XOR_n;         /* 2 */
   z80_opcodes[0xEF] = &z80_RST_28H;       /* 4 */
                                           
   z80_opcodes[0xF0] = &z80_LD_A_n;        /* 3 */
   z80_opcodes[0xF1] = &z80_POPAF;         /* 3 */
   z80_opcodes[0xF2] = &z80_LDA_AT_C;      /* 2 */
   z80_opcodes[0xF3] = &z80_DI;            /* 1 */
   z80_opcodes[0xF4] = &z80_NI;            /* 0 */
   z80_opcodes[0xF5] = &z80_PUSHAF;        /* 4 */
   z80_opcodes[0xF6] = &z80_OR_n;          /* 2 */
   z80_opcodes[0xF7] = &z80_RST_30H;       /* 4 */
   z80_opcodes[0xF8] = &z80_LDHL_SP_n;     /* 3 */
   z80_opcodes[0xF9] = &z80_LDSP_HL;       /* 2 */
   z80_opcodes[0xFA] = &z80_LDA_AT_nn;     /* 4 */
   z80_opcodes[0xFB] = &z80_EI;            /* 1 */
   z80_opcodes[0xFC] = &z80_NI;            /* 0 */
   z80_opcodes[0xFD] = &z80_NI;            /* 0 */
   z80_opcodes[0xFE] = &z80_CP_A_n;        /* 2 */
   z80_opcodes[0xFF] = &z80_RST_38H;       /* 4 */

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

   mem_load_image(z80_rom_fname);
   mem_get_rom_info();
}

void z80_advance_time(tick dt) {
   z80_ticks     += dt;
   z80_div_timer += dt;

   if (z80_div_timer >= 0xFF) {
      z80_div_timer = 0;
      mem_direct_write(DIV_REGISTER_ADDR,
            mem_direct_read(DIV_REGISTER_ADDR) + 1);
   }

   // Bit 3 enables or disables timers
   if (mem_direct_read(TIMER_CONTROL_ADDR) & 0x04) {
      z80_tima_timer += dt;
      int max = 0;
      // Bits 1-2 control the timer speed
      switch (mem_direct_read(TIMER_CONTROL_ADDR) & 0x3) {
         case 0: max = 1024; break;
         case 1: max = 16; break;
         case 2: max = 64; break;
         case 3: max = 256; break;
         default: ERROR("timer error");
      }
      while (z80_tima_timer >= max) { 
         z80_tima_timer -= max;
         mem_direct_write(TIMA_ADDR,
                          mem_direct_read(TIMA_ADDR) + 1);
         if (mem_direct_read(TIMA_ADDR) == 0) {
            mem_direct_write(INT_FLAG_ADDR,
                             mem_direct_read(INT_FLAG_ADDR) | INT_TIMA);
            mem_direct_write(TIMA_ADDR, mem_direct_read(TMA_ADDR));
         }
      }
   }
   gpu_advance_time(dt);
}

void z80_execute_step() {
   // Check interrupts
   bool interrupted = false;
   byte int_IE = mem_direct_read(INT_ENABLED_ADDR);
   byte int_IF = mem_direct_read(INT_FLAG_ADDR);
   byte irq    = int_IE & int_IF;

   if (irq != 0) {
//      z80_stop = false;
      z80_halt = false;
   }

   if (z80_ei) {
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
         z80_ei = false;
         PUSHW(z80_PC);
         z80_PC = target;
         z80_advance_time(12);
      }
   }

   if (!interrupted) {
      if (!z80_halt && !z80_stop) {
         z80_last_pc = z80_PC;
         // We already spend 4 tickcs reading the PC,
         // so instructions take 4 ticks less than listed in docs
         z80_last_op = mem_rb(z80_PC++);

         if (get_debug_time() > DEBUG_RANGE_BEGIN
          && get_debug_time() < DEBUG_RANGE_END) {
            DEBUG("%4X: %2X\tA:%2X\tSP:%4X\n",
                  z80_last_pc, z80_last_op, z80_A, z80_SP);
         }

         (*z80_opcodes[z80_last_op])();

         if (z80_ei_delay > 0) {
            z80_ei_delay--;
            if (z80_ei_delay == 0) {
               DEBUG("INTERRUPTS ENABLED\n");
               z80_ei = true;
            }
         }
      } else {
         z80_advance_time(4);
      }
   }
}

void z80_NI() {
   ERROR("Invalid instruction at PC %04X, potentially opcode %02X\n",
         z80_last_pc, z80_last_op);
}

void z80_NOP() { }

// LOAD / STORES
void z80_LDA_n()      { LOAD(z80_A, mem_rb(z80_PC++)); } 
void z80_LDB_n()      { LOAD(z80_B, mem_rb(z80_PC++)); }
void z80_LDC_n()      { LOAD(z80_C, mem_rb(z80_PC++)); }
void z80_LDD_n()      { LOAD(z80_D, mem_rb(z80_PC++)); }
void z80_LDE_n()      { LOAD(z80_E, mem_rb(z80_PC++)); }
void z80_LDH_n()      { LOAD(z80_H, mem_rb(z80_PC++)); }
void z80_LDL_n()      { LOAD(z80_L, mem_rb(z80_PC++)); }
void z80_LDA_A()      { LOAD(z80_A, z80_A); }
void z80_LDA_B()      { LOAD(z80_A, z80_B); }
void z80_LDA_C()      { LOAD(z80_A, z80_C); }
void z80_LDA_D()      { LOAD(z80_A, z80_D); }
void z80_LDA_E()      { LOAD(z80_A, z80_E); }
void z80_LDA_H()      { LOAD(z80_A, z80_H); }
void z80_LDA_L()      { LOAD(z80_A, z80_L); }
void z80_LDA_AT_HL()  { LOAD(z80_A, FETCH(z80_H, z80_L)); }
void z80_LDB_A()      { LOAD(z80_B, z80_A); }
void z80_LDB_B()      { LOAD(z80_B, z80_B); }
void z80_LDB_C()      { LOAD(z80_B, z80_C); }
void z80_LDB_D()      { LOAD(z80_B, z80_D); }
void z80_LDB_E()      { LOAD(z80_B, z80_E); }
void z80_LDB_H()      { LOAD(z80_B, z80_H); }
void z80_LDB_L()      { LOAD(z80_B, z80_L); }
void z80_LDB_AT_HL()  { LOAD(z80_B, FETCH(z80_H, z80_L)); }
void z80_LDC_A()      { LOAD(z80_C, z80_A); }
void z80_LDC_B()      { LOAD(z80_C, z80_B); }
void z80_LDC_C()      { LOAD(z80_C, z80_C); }
void z80_LDC_D()      { LOAD(z80_C, z80_D); }
void z80_LDC_E()      { LOAD(z80_C, z80_E); }
void z80_LDC_H()      { LOAD(z80_C, z80_H); }
void z80_LDC_L()      { LOAD(z80_C, z80_L); }
void z80_LDC_AT_HL()  { LOAD(z80_C, FETCH(z80_H, z80_L)); }
void z80_LDD_A()      { LOAD(z80_D, z80_A); }
void z80_LDD_B()      { LOAD(z80_D, z80_B); }
void z80_LDD_C()      { LOAD(z80_D, z80_C); }
void z80_LDD_D()      { LOAD(z80_D, z80_D); }
void z80_LDD_E()      { LOAD(z80_D, z80_E); }
void z80_LDD_H()      { LOAD(z80_D, z80_H); }
void z80_LDD_L()      { LOAD(z80_D, z80_L); }
void z80_LDD_AT_HL()  { LOAD(z80_D, FETCH(z80_H, z80_L)); }
void z80_LDE_A()      { LOAD(z80_E, z80_A); }
void z80_LDE_B()      { LOAD(z80_E, z80_B); }
void z80_LDE_C()      { LOAD(z80_E, z80_C); }
void z80_LDE_D()      { LOAD(z80_E, z80_D); }
void z80_LDE_E()      { LOAD(z80_E, z80_E); }
void z80_LDE_H()      { LOAD(z80_E, z80_H); }
void z80_LDE_L()      { LOAD(z80_E, z80_L); }
void z80_LDE_AT_HL()  { LOAD(z80_E, FETCH(z80_H, z80_L)); }
void z80_LDH_A()      { LOAD(z80_H, z80_A); }
void z80_LDH_B()      { LOAD(z80_H, z80_B); }
void z80_LDH_C()      { LOAD(z80_H, z80_C); }
void z80_LDH_D()      { LOAD(z80_H, z80_D); }
void z80_LDH_E()      { LOAD(z80_H, z80_E); }
void z80_LDH_H()      { LOAD(z80_H, z80_H); }
void z80_LDH_L()      { LOAD(z80_H, z80_L); }
void z80_LDH_AT_HL()  { LOAD(z80_H, FETCH(z80_H, z80_L)); }
void z80_LDL_A()      { LOAD(z80_L, z80_A); }
void z80_LDL_B()      { LOAD(z80_L, z80_B); }
void z80_LDL_C()      { LOAD(z80_L, z80_C); }
void z80_LDL_D()      { LOAD(z80_L, z80_D); }
void z80_LDL_E()      { LOAD(z80_L, z80_E); }
void z80_LDL_H()      { LOAD(z80_L, z80_H); }
void z80_LDL_L()      { LOAD(z80_L, z80_L); }
void z80_LDL_AT_HL()  { LOAD(z80_L, FETCH(z80_H, z80_L)); }
void z80_LD_AT_HL_B() { STORE(z80_H, z80_L, z80_B); }
void z80_LD_AT_HL_C() { STORE(z80_H, z80_L, z80_C); }
void z80_LD_AT_HL_D() { STORE(z80_H, z80_L, z80_D); }
void z80_LD_AT_HL_E() { STORE(z80_H, z80_L, z80_E); }
void z80_LD_AT_HL_H() { STORE(z80_H, z80_L, z80_H); }
void z80_LD_AT_HL_L() { STORE(z80_H, z80_L, z80_L); }
void z80_LD_AT_HL_n() { STORE(z80_H, z80_L, mem_rb(z80_PC++)); }
void z80_LDA_AT_BC()  { LOAD(z80_A, FETCH(z80_B, z80_C)); }
void z80_LDA_AT_DE()  { LOAD(z80_A, FETCH(z80_D, z80_E)); }
void z80_LD_AT_BC_A() { STORE(z80_B, z80_C, z80_A); }
void z80_LD_AT_DE_A() { STORE(z80_D, z80_E, z80_A); }
void z80_LD_AT_HL_A() { STORE(z80_H, z80_L, z80_A); }
void z80_LDA_AT_C()   { LOAD(z80_A, mem_rb(0xFF00 + z80_C)); }
void z80_LD_AT_C_A()  { STORE(0xFF, z80_C, z80_A); }

void z80_LDA_AT_nn() {
   LOAD(z80_A, mem_rb(mem_rw(z80_PC)));
   z80_PC += 2;
}

void z80_LD_AT_nn_A() {
   mem_wb(mem_rw(z80_PC), z80_A);
   z80_PC += 2;
}

void z80_LDA_AT_HLD() {
   z80_A = FETCH(z80_H, z80_L);
   z80_DEC16(&z80_H, &z80_L);
}

void z80_LDA_AT_HLI() {
   z80_A = FETCH(z80_H, z80_L);
   z80_INC16(&z80_H, &z80_L);
}

void z80_LD_AT_HLD_A() {
   STORE(z80_H, z80_L, z80_A);
   z80_DEC16(&z80_H, &z80_L);
}

void z80_LD_AT_HLI_A() {
   STORE(z80_H, z80_L, z80_A);
   z80_INC16(&z80_H, &z80_L);
}

void z80_LD_n_A() {
   STORE(0xFF, mem_rb(z80_PC++), z80_A);
}

void z80_LD_A_n() {
   z80_A = FETCH(0xFF, mem_rb(z80_PC++));
}

void z80_LDBC_nn() {
   z80_C = mem_rb(z80_PC++);
   z80_B = mem_rb(z80_PC++);
}

void z80_LDDE_nn() {
   z80_E = mem_rb(z80_PC++);
   z80_D = mem_rb(z80_PC++);
}

void z80_LDHL_nn() {
   z80_L = mem_rb(z80_PC++);
   z80_H = mem_rb(z80_PC++);
}

void z80_LDSP_nn() {
   z80_SP = mem_rw(z80_PC);
   z80_PC += 2;
}

void z80_LDSP_HL() {
   z80_SP = (z80_H << 8) | z80_L;
   z80_advance_time(8);
}

void z80_LDHL_SP_n() {
   byte  next = mem_rb(z80_PC++);
   sbyte off  = (sbyte)next;
   int   res  = off + z80_SP;
   CLEAR_FLAGS();
   FLAG_C = ((z80_SP & 0xFF) + next > 0xFF);
   FLAG_H = ((z80_SP & 0xF) + (next & 0xF) > 0xF);
   z80_H  = (res & 0xFF00) >> 8;
   z80_L  = res & 0x00FF;
   z80_advance_time(4444);
}

void z80_LD_nn_SP() {
   word tmp = mem_rw(z80_PC);
   z80_PC += 2;
   mem_ww(tmp, z80_SP);
}

// PUSH / POP

void z80_PUSHBC() { PUSH(z80_B, z80_C); }
void z80_PUSHDE() { PUSH(z80_D, z80_E); }
void z80_PUSHHL() { PUSH(z80_H, z80_L); }
void z80_POPBC() { POP(z80_B, z80_C); }
void z80_POPDE() { POP(z80_D, z80_E); }
void z80_POPHL() { POP(z80_H, z80_L); }

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
   PUSH(z80_A, flags);
}

void z80_POPAF() {
   byte flags = 0;
   POP(z80_A, flags);
   FLAG_Z = (flags & BITMASK_Z);
   FLAG_C = (flags & BITMASK_C);
   FLAG_H = (flags & BITMASK_H);
   FLAG_N = (flags & BITMASK_N);
}

// AND / OR / XOR
void z80_AND_A()     { AND(z80_A); }
void z80_AND_B()     { AND(z80_B); }
void z80_AND_C()     { AND(z80_C); }
void z80_AND_D()     { AND(z80_D); }
void z80_AND_E()     { AND(z80_E); }
void z80_AND_H()     { AND(z80_H); }
void z80_AND_L()     { AND(z80_L); }
void z80_AND_AT_HL() { AND(FETCH(z80_H, z80_L)); }
void z80_AND_n()     { AND(mem_rb(z80_PC++)); }
void z80_OR_A()      { OR(z80_A); }
void z80_OR_B()      { OR(z80_B); }
void z80_OR_C()      { OR(z80_C); }
void z80_OR_D()      { OR(z80_D); }
void z80_OR_E()      { OR(z80_E); }
void z80_OR_H()      { OR(z80_H); }
void z80_OR_L()      { OR(z80_L); }
void z80_OR_AT_HL()  { OR(FETCH(z80_H, z80_L)); }
void z80_OR_n()      { OR(mem_rb(z80_PC++)); }
void z80_XOR_A()     { XOR(z80_A); }
void z80_XOR_B()     { XOR(z80_B); }
void z80_XOR_C()     { XOR(z80_C); }
void z80_XOR_D()     { XOR(z80_D); }
void z80_XOR_E()     { XOR(z80_E); }
void z80_XOR_H()     { XOR(z80_H); }
void z80_XOR_L()     { XOR(z80_L); }
void z80_XOR_AT_HL() { XOR(FETCH(z80_H, z80_L)); }
void z80_XOR_n()     { XOR(mem_rb(z80_PC++)); }

void z80_RLC(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   FLAG_C = (t & 0x80);
   t = (t << 1) | FLAG_C;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_RRC(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   FLAG_C = (t & 0x01);
   t = (t >> 1) | (FLAG_C << 7);
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_RL(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   byte old_carry = FLAG_C;
   FLAG_C = (t & 0x80);
   t = (t << 1) | old_carry;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_RR(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   byte old_carry = FLAG_C;
   FLAG_C = (t & 0x01);
   t = (t >> 1) | (old_carry << 7);
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_SLA(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   FLAG_C = (t & 0x80);
   t = t << 1;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_SRA(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   byte msb = t & 0x80;
   FLAG_C = (t & 0x01);
   t = (t >> 1) | msb;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_SWAP(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   t = ((t << 4) | (t >> 4));
   CLEAR_FLAGS();
   FLAG_Z = (t == 0);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_SRL(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   FLAG_C = (t & 0x01);
   t = t >> 1;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_BIT(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
      // Conflicting info here- Official GB doc
      // says 12 total cycles, pan docs say 16.
      // Trying 16.
      z80_advance_time(4);
   }

   FLAG_Z = (!(t & (1 << bit)));
   FLAG_N = (false);
   FLAG_H = (true);
}

void z80_RES(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   t = t & ~(1 << bit);

   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
   } else {
      *inp = t;
   }
}

void z80_SET(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(z80_H, z80_L);
   }

   t = t | (1 << bit);


   if (inp == NULL) {
      STORE(z80_H, z80_L, t);
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
   JP();
}

void z80_JP_NZ_nn() {
   if (!FLAG_Z) {
      JP();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_JP_Z_nn() {
   if (FLAG_Z) {
      JP();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_JP_NC_nn() {
   if (!FLAG_C) {
      JP();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_JP_C_nn() {
   if (FLAG_C) {
      JP();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_JP_AT_HL() {
   z80_PC = ((z80_H << 8) | z80_L);
}

void z80_JR_n() {
   JR();
}

void z80_JR_NZ_n() {
   if (!FLAG_Z) {
      JR();
   } else {
      z80_advance_time(4);
      z80_PC++;
   }
}

void z80_JR_Z_n() {
   if (FLAG_Z) {
      JR();
   } else {
      z80_advance_time(4);
      z80_PC++;
   }
}

void z80_JR_NC_n() {
   if (!FLAG_C) {
      JR();
   } else {
      z80_advance_time(4);
      z80_PC++;
   }
}

void z80_JR_C_n() {
   if (FLAG_C) {
      JR();
   } else {
      z80_advance_time(4);
      z80_PC++;
   }
}

void z80_CALL() {
   PUSHW(z80_PC + 2);
   z80_PC = mem_rw(z80_PC);
   z80_advance_time(4);
}

void z80_CALL_nn() {
   z80_CALL();
}

void z80_CALL_NZ_nn() {
   if (!FLAG_Z) {
      z80_CALL();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_CALL_Z_nn() {
   if (FLAG_Z) {
      z80_CALL();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_CALL_NC_nn() {
   if (!FLAG_C) {
      z80_CALL();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

void z80_CALL_C_nn() {
   if (FLAG_C) {
      z80_CALL();
   } else {
      z80_advance_time(8);
      z80_PC += 2;
   }
}

// Restarts

void z80_RST_00H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x00;
   z80_advance_time(4);
}

void z80_RST_08H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x08;
   z80_advance_time(4);
}

void z80_RST_10H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x10;
   z80_advance_time(4);
}

void z80_RST_18H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x18;
   z80_advance_time(4);
}

void z80_RST_20H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x20;
   z80_advance_time(4);
}

void z80_RST_28H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x28;
   z80_advance_time(4);
}

void z80_RST_30H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x30;
   z80_advance_time(4);
}

void z80_RST_38H() {
   PUSHW(z80_PC);
   z80_halt = false;
   z80_stop = false;
   z80_PC   = 0x38;
   z80_advance_time(4);

   // If reset 38 is pointing to itself, error out
   if (mem_direct_read(z80_PC) == 0xFF) {
      ERROR("Reset 38H loop detected.\n");
   }
}

// Returns

void z80_RETURN() {
   POPW(z80_PC);
   z80_advance_time(4);
}

void z80_RET() {
   z80_RETURN();
}

void z80_RET_NZ() {
   if (!FLAG_Z) {
      z80_RETURN();
   } else {
      z80_advance_time(4);
   }
}

void z80_RET_Z() {
   if (FLAG_Z) {
      z80_RETURN();
   } else {
      z80_advance_time(4);
   }
}

void z80_RET_NC() {
   if (!FLAG_C) {
      z80_RETURN();
   } else {
      z80_advance_time(4);
   }
}

void z80_RET_C() {
   if (FLAG_C) {
      z80_RETURN();
   } else {
      z80_advance_time(4);
   }
}

void z80_RETI() {
   z80_ei = true;
   z80_RETURN();
}

// 16 bit math

void z80_ADD16(byte* a_hi, byte* a_low, byte b_hi, byte b_low) {
   word a = (*a_hi << 8) | *a_low;
   word b = (b_hi << 8) | b_low;

   uint32_t carryCheck = a + b;
   FLAG_H = ((0x0FFF & a) + (0x0FFF & b) > 0x0FFF);
   FLAG_C = (carryCheck > 0x0000FFFF);
   FLAG_N = (false);

   a += b;
   *a_hi  = (a & 0xFF00) >> 8;
   *a_low = a & 0x00FF;
   z80_advance_time(4);
}

void z80_INC16(byte* hi, byte* low) {
   if ((*low) == 0xFF) {
      (*hi)++;
   }
   (*low)++;
}

void z80_DEC16(byte* hi, byte* low) {
   if ((*low) == 0) {
      (*hi)--;
   }
   (*low)--;
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
   CLEAR_FLAGS();
   FLAG_H = ((z80_SP & 0xF) + (val & 0xF) > 0xF);
   FLAG_C = (((z80_SP & 0xFF) + val > 0xFF));
   z80_SP += off;
   z80_advance_time(8);
}

// 0x03
void z80_INC16_BC() {
   z80_INC16(&z80_B, &z80_C);
   z80_advance_time(4);
}

void z80_INC16_DE() {
   z80_INC16(&z80_D, &z80_E);
   z80_advance_time(4);
}

void z80_INC16_HL() {
   z80_INC16(&z80_H, &z80_L);
   z80_advance_time(4);
}

void z80_INC16_SP() {
   z80_SP++;
   z80_advance_time(4);
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
   z80_advance_time(4);
}

// Rotates / misc

void z80_RLA() {
   z80_RL(&z80_A);
   FLAG_Z = false;
}

void z80_RLCA() {
   z80_RLC(&z80_A);
   FLAG_Z = false;
}

void z80_RRCA() {
   z80_RRC(&z80_A);
   FLAG_Z = false;
}

void z80_RRA() {
   z80_RR(&z80_A);
   FLAG_Z = false;
}

void z80_DI() {
   z80_ei = false;
   DEBUG("INTERRUPTS DISABLED\n");
}

void z80_EI() {
   // EI Disables interrupts for one instruction, then enables them
   z80_ei_delay = 2;
   z80_ei       = false;
}

// ADD / ADC / SUB / SUBC
void z80_ADD_A_A()     { ADD(z80_A); }
void z80_ADD_A_B()     { ADD(z80_B); }
void z80_ADD_A_C()     { ADD(z80_C); }
void z80_ADD_A_D()     { ADD(z80_D); }
void z80_ADD_A_E()     { ADD(z80_E); }
void z80_ADD_A_H()     { ADD(z80_H); }
void z80_ADD_A_L()     { ADD(z80_L); }
void z80_ADD_A_AT_HL() { ADD(FETCH(z80_H, z80_L)); }
void z80_ADD_A_n()     { ADD(mem_rb(z80_PC++)); }
void z80_ADC_A_A()     { ADC(z80_A); }
void z80_ADC_A_B()     { ADC(z80_B); }
void z80_ADC_A_C()     { ADC(z80_C); }
void z80_ADC_A_D()     { ADC(z80_D); }
void z80_ADC_A_E()     { ADC(z80_E); }
void z80_ADC_A_H()     { ADC(z80_H); }
void z80_ADC_A_L()     { ADC(z80_L); }
void z80_ADC_A_AT_HL() { ADC(FETCH(z80_H, z80_L)); }
void z80_ADC_A_n()     { ADC(mem_rb(z80_PC++)); }
void z80_SUB_A_A()     { SUB(z80_A); }
void z80_SUB_A_B()     { SUB(z80_B); }
void z80_SUB_A_C()     { SUB(z80_C); }
void z80_SUB_A_D()     { SUB(z80_D); }
void z80_SUB_A_E()     { SUB(z80_E); }
void z80_SUB_A_H()     { SUB(z80_H); }
void z80_SUB_A_L()     { SUB(z80_L); }
void z80_SUB_A_AT_HL() { SUB(FETCH(z80_H, z80_L)); }
void z80_SUB_A_n()     { SUB(mem_rb(z80_PC++)); }
void z80_SBC_A_A()     { SBC(z80_A); }
void z80_SBC_A_B()     { SBC(z80_B); }
void z80_SBC_A_C()     { SBC(z80_C); }
void z80_SBC_A_D()     { SBC(z80_D); }
void z80_SBC_A_E()     { SBC(z80_E); }
void z80_SBC_A_H()     { SBC(z80_H); }
void z80_SBC_A_L()     { SBC(z80_L); }
void z80_SBC_A_AT_HL() { SBC(FETCH(z80_H, z80_L)); }
void z80_SBC_A_n()     { SBC(mem_rb(z80_PC++)); }

// INCREMENT / DECREMENT
void z80_INC_A() { INC(z80_A); }
void z80_INC_B() { INC(z80_B); }
void z80_INC_C() { INC(z80_C); }
void z80_INC_D() { INC(z80_D); }
void z80_INC_E() { INC(z80_E); }
void z80_INC_H() { INC(z80_H); }
void z80_INC_L() { INC(z80_L); }
void z80_DEC_A() { DEC(z80_A); }
void z80_DEC_B() { DEC(z80_B); }
void z80_DEC_C() { DEC(z80_C); }
void z80_DEC_D() { DEC(z80_D); }
void z80_DEC_E() { DEC(z80_E); }
void z80_DEC_H() { DEC(z80_H); }
void z80_DEC_L() { DEC(z80_L); }

void z80_INC_AT_HL() {
   byte val = FETCH(z80_H, z80_L);
   FLAG_H = (val & 0x0F) + 1 > 0x0F; 
   FLAG_N = false; 
   val++; 
   FLAG_Z = val == 0; 
   STORE(z80_H, z80_L, val);
}

void z80_DEC_AT_HL() {
   byte val = FETCH(z80_H, z80_L);
   FLAG_H = (val & 0x0F) == 0; 
   val--; 
   FLAG_Z = val == 0; 
   FLAG_N = true; 
   STORE(z80_H, z80_L, val);
}

// COMPARE
void z80_CP_A()      { COMPARE(z80_A); }
void z80_CP_B()      { COMPARE(z80_B); }
void z80_CP_C()      { COMPARE(z80_C); }
void z80_CP_D()      { COMPARE(z80_D); }
void z80_CP_E()      { COMPARE(z80_E); }
void z80_CP_H()      { COMPARE(z80_H); }
void z80_CP_L()      { COMPARE(z80_L); }
void z80_CP_AT_HL()  { COMPARE(FETCH(z80_H, z80_L)); }
void z80_CP_A_n()    { COMPARE(mem_rb(z80_PC++)); }

void z80_CPL() {
   z80_A = ~z80_A;
   FLAG_H = true;
   FLAG_N = true;
}

void z80_CCF() {
   FLAG_C = !FLAG_C;
   FLAG_N = false;
   FLAG_H = false;
}

void z80_SCF() {
   FLAG_H = false;
   FLAG_N = false;
   FLAG_C = true;
}

void z80_HALT() {
   z80_halt = true;
   DEBUG("HALT\n");
}

void z80_STOP() {
   z80_stop = true;
   DEBUG("STOP\n");
}

void z80_DAA() {
   word a = z80_A;
   if (!FLAG_N) {
      if (FLAG_H || (a & 0x0F) > 0x09) {
         a += 0x06;
      }
      if (FLAG_C || a > 0x9F) {
         a += 0x60;
      }
   } else {
      if (FLAG_H) {
         a = (a - 0x06) & 0xFF;
      }
      if (FLAG_C) {
         a -= 0x60;
      }
   }

   FLAG_H = (false);
   FLAG_C = (a & 0x0100);
   FLAG_Z = (a == 0);
   z80_A = 0xFF & a;
}
