#ifndef __CPU_H__
#define __CPU_H__

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
byte cpu_last_op;
word cpu_last_pc;

// Registers
byte  cpu_A;
byte  cpu_B;
byte  cpu_C;
byte  cpu_D;
byte  cpu_E;
byte  cpu_H;
byte  cpu_L;
byte  cpu_ei_delay;
word  cpu_PC; // Program Counter
word  cpu_SP; // Stack Pointer
bool  cpu_ei;
bool  cpu_halt;
bool  cpu_stop;
tick  cpu_ticks;
tick  cpu_div_timer;
tick  cpu_tima_timer;
char* cpu_rom_fname;
void (*cpu_opcodes[0x100])();

void cpu_execute_step();
void cpu_init(char* romname);
void cpu_reset();
void cpu_advance_time(tick dt);

// And now, instructions
void cpu_NI();
void cpu_NOP();
void cpu_CALL();
void cpu_RETURN();

byte cpu_get_flag_zero();
byte cpu_get_flag_carry();
byte cpu_get_flag_halfcarry();
byte cpu_get_flag_operation();

void cpu_CB();

void cpu_RLC(byte* inp);
void cpu_RRC(byte* inp);
void cpu_RL(byte* inp);
void cpu_RR(byte* inp);
void cpu_SLA(byte* inp);
void cpu_SRA(byte* inp);
void cpu_SWAP(byte* inp);
void cpu_SRL(byte* inp);
void cpu_BIT(byte* inp, byte bit);
void cpu_RES(byte* inp, byte bit);
void cpu_SET(byte* inp, byte bit);

// LOAD / STORES

void cpu_LDB_n();       /*0x06*/
void cpu_LDC_n();       /*0x0E*/
void cpu_LDD_n();       /*0x16*/
void cpu_LDE_n();       /*0x1E*/
void cpu_LDH_n();       /*0x26*/
void cpu_LDL_n();       /*0x2E*/
void cpu_LDA_A();       /*0x7F*/
void cpu_LDA_B();       /*0x78*/
void cpu_LDA_C();       /*0x79*/
void cpu_LDA_D();       /*0x7A*/
void cpu_LDA_E();       /*0x7B*/
void cpu_LDA_H();       /*0x7C*/
void cpu_LDA_L();       /*0x7D*/
void cpu_LDA_AT_HL();   /*0x7E*/
void cpu_LDB_B();       /*0x40*/
void cpu_LDB_C();       /*0x41*/
void cpu_LDB_D();       /*0x42*/
void cpu_LDB_E();       /*0x43*/
void cpu_LDB_H();       /*0x44*/
void cpu_LDB_L();       /*0x45*/
void cpu_LDB_AT_HL();   /*0x46*/
void cpu_LDC_B();       /*0x48*/
void cpu_LDC_C();       /*0x49*/
void cpu_LDC_D();       /*0x4A*/
void cpu_LDC_E();       /*0x4B*/
void cpu_LDC_H();       /*0x4C*/
void cpu_LDC_L();       /*0x4D*/
void cpu_LDC_AT_HL();   /*0x4E*/
void cpu_LDD_B();       /*0x50*/
void cpu_LDD_C();       /*0x51*/
void cpu_LDD_D();       /*0x52*/
void cpu_LDD_E();       /*0x53*/
void cpu_LDD_H();       /*0x54*/
void cpu_LDD_L();       /*0x55*/
void cpu_LDD_AT_HL();   /*0x56*/
void cpu_LDE_B();       /*0x58*/
void cpu_LDE_C();       /*0x59*/
void cpu_LDE_D();       /*0x5A*/
void cpu_LDE_E();       /*0x5B*/
void cpu_LDE_H();       /*0x5C*/
void cpu_LDE_L();       /*0x5D*/
void cpu_LDE_AT_HL();   /*0x5E*/
void cpu_LDH_B();       /*0x60*/
void cpu_LDH_C();       /*0x61*/
void cpu_LDH_D();       /*0x62*/
void cpu_LDH_E();       /*0x63*/
void cpu_LDH_H();       /*0x64*/
void cpu_LDH_L();       /*0x65*/
void cpu_LDH_AT_HL();   /*0x66*/
void cpu_LDL_B();       /*0x68*/
void cpu_LDL_C();       /*0x69*/
void cpu_LDL_D();       /*0x6A*/
void cpu_LDL_E();       /*0x6B*/
void cpu_LDL_H();       /*0x6C*/
void cpu_LDL_L();       /*0x6D*/
void cpu_LDL_AT_HL();   /*0x6E*/
void cpu_LD_AT_HL_B();  /*0x70*/
void cpu_LD_AT_HL_C();  /*0x71*/
void cpu_LD_AT_HL_D();  /*0x72*/
void cpu_LD_AT_HL_E();  /*0x73*/
void cpu_LD_AT_HL_H();  /*0x74*/
void cpu_LD_AT_HL_L();  /*0x75*/
void cpu_LD_AT_HL_n();  /*0x36*/
void cpu_LDA_AT_BC();   /*0x0A*/
void cpu_LDA_AT_DE();   /*0x1A*/
void cpu_LDA_AT_nn();   /*0xFA*/
void cpu_LDA_n();       /*0x3E*/
void cpu_LDB_A();       /*0x47*/
void cpu_LDC_A();       /*0x4F*/
void cpu_LDD_A();       /*0x57*/
void cpu_LDE_A();       /*0x5F*/
void cpu_LDH_A();       /*0x67*/
void cpu_LDL_A();       /*0x6F*/
void cpu_LD_AT_BC_A();  /*0x02*/
void cpu_LD_AT_DE_A();  /*0x12*/
void cpu_LD_AT_HL_A();  /*0x77*/
void cpu_LD_AT_nn_A();  /*0xEA*/
void cpu_LDA_AT_C();    /*0xF2*/
void cpu_LD_AT_C_A();   /*0xE2*/
void cpu_LDA_AT_HLD();  /*0x3A*/
void cpu_LD_AT_HLD_A(); /*0x32*/
void cpu_LDA_AT_HLI();  /*0x2A*/
void cpu_LD_AT_HLI_A(); /*0x22*/
void cpu_LD_n_A();      /*0xE0*/
void cpu_LD_A_n();      /*0xF0*/
void cpu_LDBC_nn();     /*0x01*/
void cpu_LDDE_nn();     /*0x11*/
void cpu_LDHL_nn();     /*0x21*/
void cpu_LDSP_nn();     /*0x31*/
void cpu_LDSP_HL();     /*0xF9*/
void cpu_LDHL_SP_n();   /*0xF8*/
void cpu_LD_nn_SP();    /*0x08*/

// PUSH / POP

void cpu_PUSHAF(); /*F5*/
void cpu_PUSHBC(); /*C5*/
void cpu_PUSHDE(); /*D5*/
void cpu_PUSHHL(); /*E5*/
void cpu_POPAF();  /*F1*/
void cpu_POPBC();  /*C1*/
void cpu_POPDE();  /*D1*/
void cpu_POPHL();  /*E1*/

// AND / OR / XOR

void cpu_AND_A(); /*A7*/
void cpu_AND_B();
void cpu_AND_C();
void cpu_AND_D();
void cpu_AND_E();
void cpu_AND_H();
void cpu_AND_L();
void cpu_AND_AT_HL();
void cpu_AND_n();
void cpu_OR_A();
void cpu_OR_B();
void cpu_OR_C();
void cpu_OR_D();
void cpu_OR_E();
void cpu_OR_H();
void cpu_OR_L();
void cpu_OR_AT_HL();
void cpu_OR_n();
void cpu_XOR_A();
void cpu_XOR_B();
void cpu_XOR_C();
void cpu_XOR_D();
void cpu_XOR_E();
void cpu_XOR_H();
void cpu_XOR_L();
void cpu_XOR_AT_HL();
void cpu_XOR_n();

// JUMP / RETURN

void cpu_JP_nn();
void cpu_JP_NZ_nn();
void cpu_JP_Z_nn();
void cpu_JP_NC_nn();
void cpu_JP_C_nn();
void cpu_JP_AT_HL();
void cpu_JR_n();
void cpu_JR_NZ_n();
void cpu_JR_Z_n();
void cpu_JR_NC_n();
void cpu_JR_C_n();
void cpu_CALL_nn();
void cpu_CALL_NZ_nn();
void cpu_CALL_Z_nn();
void cpu_CALL_NC_nn();
void cpu_CALL_C_nn();
void cpu_RST_00H();
void cpu_RST_08H();
void cpu_RST_10H();
void cpu_RST_18H();
void cpu_RST_20H();
void cpu_RST_28H();
void cpu_RST_30H();
void cpu_RST_38H();
void cpu_RET();
void cpu_RET_NZ(); // Clock might be wrong on these...
void cpu_RET_Z();
void cpu_RET_NC();
void cpu_RET_C();
void cpu_RETI();

void cpu_ADD16(byte* a_hi, byte* a_low, byte b_hi, byte b_low);
void cpu_INC16(byte* hi, byte* low);
void cpu_DEC16(byte* hi, byte* low);

void cpu_ADD16_HL_BC(); /*0x09*/
void cpu_ADD16_HL_DE(); /*0x19*/
void cpu_ADD16_HL_HL(); /*0x29*/
void cpu_ADD16_HL_SP(); /*0x39*/
void cpu_ADD16_SP_n();  /*0xE8*/

void cpu_INC16_BC(); /*0x03*/
void cpu_INC16_DE(); /*0x13*/
void cpu_INC16_HL(); /*0x23*/
void cpu_INC16_SP(); /*0x33*/
void cpu_DEC16_BC(); /*0x0B*/
void cpu_DEC16_DE(); /*0x1B*/
void cpu_DEC16_HL(); /*0x2B*/
void cpu_DEC16_SP(); /*0x3B*/

void cpu_RLA();
void cpu_RLCA();
void cpu_RRCA();
void cpu_RRA();
void cpu_DI();
void cpu_EI();

// ADD / ADC / SUB / SUBC

void cpu_ADD_A_A();     /*0x87*/
void cpu_ADD_A_B();     /*0x80*/
void cpu_ADD_A_C();     /*0x81*/
void cpu_ADD_A_D();     /*0x82*/
void cpu_ADD_A_E();     /*0x83*/
void cpu_ADD_A_H();     /*0x84*/
void cpu_ADD_A_L();     /*0x85*/
void cpu_ADD_A_AT_HL(); /*0x86*/
void cpu_ADD_A_n();     /*0xC6*/
void cpu_ADC_A_A();     /*0x8F*/
void cpu_ADC_A_B();     /*0x88*/
void cpu_ADC_A_C();     /*0x89*/
void cpu_ADC_A_D();     /*0x8A*/
void cpu_ADC_A_E();     /*0x8B*/
void cpu_ADC_A_H();     /*0x8C*/
void cpu_ADC_A_L();     /*0x8D*/
void cpu_ADC_A_AT_HL(); /*0x8E*/
void cpu_ADC_A_n();     /*0xCE*/
void cpu_SUB_A_A();
void cpu_SUB_A_B();
void cpu_SUB_A_C();
void cpu_SUB_A_D();
void cpu_SUB_A_E();
void cpu_SUB_A_H();
void cpu_SUB_A_L();
void cpu_SUB_A_AT_HL();
void cpu_SUB_A_n();
void cpu_SBC_A_A();
void cpu_SBC_A_B();
void cpu_SBC_A_C();
void cpu_SBC_A_D();
void cpu_SBC_A_E();
void cpu_SBC_A_H();
void cpu_SBC_A_L();
void cpu_SBC_A_AT_HL();
void cpu_SBC_A_n();

// INCREMENT / DECREMENT

void cpu_INC_A();
void cpu_INC_B();
void cpu_INC_C();
void cpu_INC_D();
void cpu_INC_E();
void cpu_INC_H();
void cpu_INC_L();
void cpu_INC_AT_HL();
void cpu_DEC_A();
void cpu_DEC_B();
void cpu_DEC_C();
void cpu_DEC_D();
void cpu_DEC_E();
void cpu_DEC_H();
void cpu_DEC_L();
void cpu_DEC_AT_HL();

// COMPARE

void cpu_CP_A();
void cpu_CP_B();
void cpu_CP_C();
void cpu_CP_D();
void cpu_CP_E();
void cpu_CP_H();
void cpu_CP_L();
void cpu_CP_AT_HL();
void cpu_CP_A_n();

void cpu_CPL();
void cpu_CCF();
void cpu_SCF();
void cpu_HALT();
void cpu_STOP();

void cpu_DAA();

#endif
