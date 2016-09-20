#ifndef __Z80_H__
#define __Z80_H__

#include "datatypes.h"
#include "memory.h"

#define BITMASK_C 0x10
#define BITMASK_H 0x20
#define BITMASK_N 0x40
#define BITMASK_Z 0x80
#define INT_VBLANK 0x01
#define INT_STAT 0x02
#define INT_TIMA 0x04
#define INT_INPUT 0x10

// For debugging
byte z80_last_op;
word z80_last_pc;

// Registers
byte  z80_A;
byte  z80_B;
byte  z80_C;
byte  z80_D;
byte  z80_E;
byte  z80_H;
byte  z80_L;
byte  z80_ei_delay;
word  z80_PC; // Program Counter
word  z80_SP; // Stack Pointer
bool  z80_ei;
bool  z80_halt;
bool  z80_stop;
tick  z80_ticks;
tick  z80_dt;
tick  z80_div_timer;
tick  z80_tima_timer;
char* z80_rom_fname;
void (*z80_opcodes[0x100])();

tick z80_execute_step();
void z80_init(char* romname);
void z80_reset();
void z80_clear_flags();
void z80_set_flag_zero(bool tf);
void z80_set_flag_carry(bool tf);
void z80_set_flag_halfcarry(bool tf);
void z80_set_flag_operation(bool tf);

// And now, instructions
void z80_NI();
void z80_NOP();
void z80_CALL();
void z80_RETURN();

byte z80_get_flag_zero();
byte z80_get_flag_carry();
byte z80_get_flag_halfcarry();
byte z80_get_flag_operation();

void z80_CB();

void z80_RLC(byte* inp);
void z80_RRC(byte* inp);
void z80_RL(byte* inp);
void z80_RR(byte* inp);
void z80_SLA(byte* inp);
void z80_SRA(byte* inp);
void z80_SWAP(byte* inp);
void z80_SRL(byte* inp);
void z80_BIT(byte* inp, byte bit);
void z80_RES(byte* inp, byte bit);
void z80_SET(byte* inp, byte bit);

// LOAD / STORES

void z80_LDB_n();       /*0x06*/
void z80_LDC_n();       /*0x0E*/
void z80_LDD_n();       /*0x16*/
void z80_LDE_n();       /*0x1E*/
void z80_LDH_n();       /*0x26*/
void z80_LDL_n();       /*0x2E*/
void z80_LDA_A();       /*0x7F*/
void z80_LDA_B();       /*0x78*/
void z80_LDA_C();       /*0x79*/
void z80_LDA_D();       /*0x7A*/
void z80_LDA_E();       /*0x7B*/
void z80_LDA_H();       /*0x7C*/
void z80_LDA_L();       /*0x7D*/
void z80_LDA_AT_HL();   /*0x7E*/
void z80_LDB_B();       /*0x40*/
void z80_LDB_C();       /*0x41*/
void z80_LDB_D();       /*0x42*/
void z80_LDB_E();       /*0x43*/
void z80_LDB_H();       /*0x44*/
void z80_LDB_L();       /*0x45*/
void z80_LDB_AT_HL();   /*0x46*/
void z80_LDC_B();       /*0x48*/
void z80_LDC_C();       /*0x49*/
void z80_LDC_D();       /*0x4A*/
void z80_LDC_E();       /*0x4B*/
void z80_LDC_H();       /*0x4C*/
void z80_LDC_L();       /*0x4D*/
void z80_LDC_AT_HL();   /*0x4E*/
void z80_LDD_B();       /*0x50*/
void z80_LDD_C();       /*0x51*/
void z80_LDD_D();       /*0x52*/
void z80_LDD_E();       /*0x53*/
void z80_LDD_H();       /*0x54*/
void z80_LDD_L();       /*0x55*/
void z80_LDD_AT_HL();   /*0x56*/
void z80_LDE_B();       /*0x58*/
void z80_LDE_C();       /*0x59*/
void z80_LDE_D();       /*0x5A*/
void z80_LDE_E();       /*0x5B*/
void z80_LDE_H();       /*0x5C*/
void z80_LDE_L();       /*0x5D*/
void z80_LDE_AT_HL();   /*0x5E*/
void z80_LDH_B();       /*0x60*/
void z80_LDH_C();       /*0x61*/
void z80_LDH_D();       /*0x62*/
void z80_LDH_E();       /*0x63*/
void z80_LDH_H();       /*0x64*/
void z80_LDH_L();       /*0x65*/
void z80_LDH_AT_HL();   /*0x66*/
void z80_LDL_B();       /*0x68*/
void z80_LDL_C();       /*0x69*/
void z80_LDL_D();       /*0x6A*/
void z80_LDL_E();       /*0x6B*/
void z80_LDL_H();       /*0x6C*/
void z80_LDL_L();       /*0x6D*/
void z80_LDL_AT_HL();   /*0x6E*/
void z80_LD_AT_HL_B();  /*0x70*/
void z80_LD_AT_HL_C();  /*0x71*/
void z80_LD_AT_HL_D();  /*0x72*/
void z80_LD_AT_HL_E();  /*0x73*/
void z80_LD_AT_HL_H();  /*0x74*/
void z80_LD_AT_HL_L();  /*0x75*/
void z80_LD_AT_HL_n();  /*0x36*/
void z80_LDA_AT_BC();   /*0x0A*/
void z80_LDA_AT_DE();   /*0x1A*/
void z80_LDA_AT_nn();   /*0xFA*/
void z80_LDA_n();       /*0x3E*/
void z80_LDB_A();       /*0x47*/
void z80_LDC_A();       /*0x4F*/
void z80_LDD_A();       /*0x57*/
void z80_LDE_A();       /*0x5F*/
void z80_LDH_A();       /*0x67*/
void z80_LDL_A();       /*0x6F*/
void z80_LD_AT_BC_A();  /*0x02*/
void z80_LD_AT_DE_A();  /*0x12*/
void z80_LD_AT_HL_A();  /*0x77*/
void z80_LD_AT_nn_A();  /*0xEA*/
void z80_LDA_AT_C();    /*0xF2*/
void z80_LD_AT_C_A();   /*0xE2*/
void z80_LDA_AT_HLD();  /*0x3A*/
void z80_LD_AT_HLD_A(); /*0x32*/
void z80_LDA_AT_HLI();  /*0x2A*/
void z80_LD_AT_HLI_A(); /*0x22*/
void z80_LD_n_A();      /*0xE0*/
void z80_LD_A_n();      /*0xF0*/
void z80_LDBC_nn();     /*0x01*/
void z80_LDDE_nn();     /*0x11*/
void z80_LDHL_nn();     /*0x21*/
void z80_LDSP_nn();     /*0x31*/
void z80_LDSP_HL();     /*0xF9*/
void z80_LDHL_SP_n();   /*0xF8*/
void z80_LD_nn_SP();    /*0x08*/

// PUSH / POP

void z80_PUSH(byte hi, byte low);
void z80_POP(byte* hi, byte* low);

void z80_PUSHAF(); /*F5*/
void z80_PUSHBC(); /*C5*/
void z80_PUSHDE(); /*D5*/
void z80_PUSHHL(); /*E5*/
void z80_POPAF();  /*F1*/
void z80_POPBC();  /*C1*/
void z80_POPDE();  /*D1*/
void z80_POPHL();  /*E1*/

// AND / OR / XOR

void z80_AND(byte inp);
void z80_OR(byte inp);
void z80_XOR(byte inp);

void z80_AND_A(); /*A7*/
void z80_AND_B();
void z80_AND_C();
void z80_AND_D();
void z80_AND_E();
void z80_AND_H();
void z80_AND_L();
void z80_AND_AT_HL();
void z80_AND_n();
void z80_OR_A();
void z80_OR_B();
void z80_OR_C();
void z80_OR_D();
void z80_OR_E();
void z80_OR_H();
void z80_OR_L();
void z80_OR_AT_HL();
void z80_OR_n();
void z80_XOR_A();
void z80_XOR_B();
void z80_XOR_C();
void z80_XOR_D();
void z80_XOR_E();
void z80_XOR_H();
void z80_XOR_L();
void z80_XOR_AT_HL();
void z80_XOR_n();

// JUMP / RETURN

void z80_JP_nn();
void z80_JP_NZ_nn();
void z80_JP_Z_nn();
void z80_JP_NC_nn();
void z80_JP_C_nn();
void z80_JP_AT_HL();
void z80_JR_n();
void z80_JR_NZ_n();
void z80_JR_Z_n();
void z80_JR_NC_n();
void z80_JR_C_n();
void z80_CALL_nn();
void z80_CALL_NZ_nn();
void z80_CALL_Z_nn();
void z80_CALL_NC_nn();
void z80_CALL_C_nn();
void z80_RST_00H();
void z80_RST_08H();
void z80_RST_10H();
void z80_RST_18H();
void z80_RST_20H();
void z80_RST_28H();
void z80_RST_30H();
void z80_RST_38H();
void z80_RET();
void z80_RET_NZ(); // Clock might be wrong on these...
void z80_RET_Z();
void z80_RET_NC();
void z80_RET_C();
void z80_RETI();

void z80_ADD16(byte* a_hi, byte* a_low, byte b_hi, byte b_low);
void z80_INC16(byte* hi, byte* low);
void z80_DEC16(byte* hi, byte* low);

void z80_ADD16_HL_BC(); /*0x09*/
void z80_ADD16_HL_DE(); /*0x19*/
void z80_ADD16_HL_HL(); /*0x29*/
void z80_ADD16_HL_SP(); /*0x39*/
void z80_ADD16_SP_n();  /*0xE8*/

void z80_INC16_BC(); /*0x03*/
void z80_INC16_DE(); /*0x13*/
void z80_INC16_HL(); /*0x23*/
void z80_INC16_SP(); /*0x33*/
void z80_DEC16_BC(); /*0x0B*/
void z80_DEC16_DE(); /*0x1B*/
void z80_DEC16_HL(); /*0x2B*/
void z80_DEC16_SP(); /*0x3B*/

void z80_RLA();
void z80_RLCA();
void z80_RRCA();
void z80_RRA();
void z80_DI();
void z80_EI();

// ADD / ADC / SUB / SUBC

void z80_ADD(byte inp);
void z80_ADC(byte inp);
void z80_SUB(byte inp);
void z80_SBC(byte inp);

void z80_ADD_A_A();     /*0x87*/
void z80_ADD_A_B();     /*0x80*/
void z80_ADD_A_C();     /*0x81*/
void z80_ADD_A_D();     /*0x82*/
void z80_ADD_A_E();     /*0x83*/
void z80_ADD_A_H();     /*0x84*/
void z80_ADD_A_L();     /*0x85*/
void z80_ADD_A_AT_HL(); /*0x86*/
void z80_ADD_A_n();     /*0xC6*/
void z80_ADC_A_A();     /*0x8F*/
void z80_ADC_A_B();     /*0x88*/
void z80_ADC_A_C();     /*0x89*/
void z80_ADC_A_D();     /*0x8A*/
void z80_ADC_A_E();     /*0x8B*/
void z80_ADC_A_H();     /*0x8C*/
void z80_ADC_A_L();     /*0x8D*/
void z80_ADC_A_AT_HL(); /*0x8E*/
void z80_ADC_A_n();     /*0xCE*/
void z80_SUB_A_A();
void z80_SUB_A_B();
void z80_SUB_A_C();
void z80_SUB_A_D();
void z80_SUB_A_E();
void z80_SUB_A_H();
void z80_SUB_A_L();
void z80_SUB_A_AT_HL();
void z80_SUB_A_n();
void z80_SBC_A_A();
void z80_SBC_A_B();
void z80_SBC_A_C();
void z80_SBC_A_D();
void z80_SBC_A_E();
void z80_SBC_A_H();
void z80_SBC_A_L();
void z80_SBC_A_AT_HL();
void z80_SBC_A_n();

// INCREMENT / DECREMENT

void z80_INC(byte* reg);
void z80_DEC(byte* reg);

void z80_INC_A();
void z80_INC_B();
void z80_INC_C();
void z80_INC_D();
void z80_INC_E();
void z80_INC_H();
void z80_INC_L();
void z80_INC_AT_HL();
void z80_DEC_A();
void z80_DEC_B();
void z80_DEC_C();
void z80_DEC_D();
void z80_DEC_E();
void z80_DEC_H();
void z80_DEC_L();
void z80_DEC_AT_HL();

// COMPARE

void z80_COMPARE(byte inp);

void z80_CP_A();
void z80_CP_B();
void z80_CP_C();
void z80_CP_D();
void z80_CP_E();
void z80_CP_H();
void z80_CP_L();
void z80_CP_AT_HL();
void z80_CP_A_n();

void z80_CPL();
void z80_CCF();
void z80_SCF();
void z80_HALT();
void z80_STOP();

void z80_DAA();

#endif
