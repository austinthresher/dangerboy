// Opcode macros
#ifndef __OPCODES_H__
#define __OPCODES_H__

#define TIME(x) cpu_advance_time((x)*4)

#define LOAD(reg, val) reg = (val);

#define STORE(hi, lo, val) wbyte((((hi)&0xFF) << 8) | ((lo)&0xFF), (val));

#define FETCH(hi, lo) rbyte((((hi)&0xFF) << 8) | ((lo)&0xFF))

#define AND(val)             \
   cpu_a  = cpu_a & (val);   \
   FLAG_C = FLAG_N = false;  \
   FLAG_Z          = !cpu_a; \
   FLAG_H          = true;

#define OR(val)                      \
   cpu_a  = cpu_a | (val);           \
   FLAG_C = FLAG_H = FLAG_N = false; \
   FLAG_Z                   = !cpu_a;

#define XOR(val)                     \
   cpu_a  = cpu_a ^ (val);           \
   FLAG_C = FLAG_H = FLAG_N = false; \
   FLAG_Z                   = !cpu_a;

#define PUSH(hi, lo)      \
   TIME(1);               \
   wbyte(--cpu_sp, (hi)); \
   TIME(1);               \
   wbyte(--cpu_sp, (lo));

#define POP(hi, lo)      \
   TIME(1);              \
   lo = rbyte(cpu_sp++); \
   TIME(1);              \
   hi = rbyte(cpu_sp++);

#define PUSHW(val)                       \
   TIME(1);                              \
   wbyte(--cpu_sp, ((val) >> 8) & 0xFF); \
   TIME(1);                              \
   wbyte(--cpu_sp, (val)&0xFF);

#define POPW(val)          \
   TIME(1);                \
   val &= 0xFF00;          \
   val |= rbyte(cpu_sp++); \
   TIME(1);                \
   val &= 0x00FF;          \
   val |= rbyte(cpu_sp++) << 8;

#define JR() cpu_pc += (sbyte)rbyte(cpu_pc) + 1;

#define ADD(val)                                    \
   byte v   = (val);                                \
   FLAG_N   = false;                                \
   FLAG_H   = ((cpu_a & 0x0F) + (v & 0x0F)) > 0x0F; \
   word tmp = cpu_a + v;                            \
   cpu_a    = tmp & 0xFF;                           \
   FLAG_C   = tmp > 0xFF;                           \
   FLAG_Z   = cpu_a == 0;

#define ADC(val)                                           \
   byte v   = (val);                                       \
   FLAG_N   = false;                                       \
   word tmp = cpu_a + v + FLAG_C;                          \
   FLAG_H   = (cpu_a & 0x0F) + (v & 0x0F) + FLAG_C > 0x0F; \
   FLAG_C   = tmp > 0xFF;                                  \
   cpu_a    = tmp & 0xFF;                                  \
   FLAG_Z   = cpu_a == 0;

#define SUB(val)                         \
   byte v = (val);                       \
   FLAG_N = true;                        \
   FLAG_H = (cpu_a & 0x0F) < (v & 0x0F); \
   FLAG_C = cpu_a < v;                   \
   cpu_a -= v;                           \
   FLAG_Z = cpu_a == 0;

#define SBC(val)                                    \
   byte v   = (val);                                \
   FLAG_N   = true;                                 \
   byte tmp = cpu_a - v - FLAG_C;                   \
   FLAG_H   = (cpu_a & 0x0F) < (v & 0x0F) + FLAG_C; \
   FLAG_C   = cpu_a < v + FLAG_C;                   \
   cpu_a    = tmp;                                  \
   FLAG_Z   = cpu_a == 0;

#define INC(dst)                     \
   FLAG_H = ((dst)&0x0F) + 1 > 0x0F; \
   FLAG_N = false;                   \
   dst++;                            \
   FLAG_Z = (dst) == 0;

#define DEC(dst)               \
   FLAG_H = ((dst)&0x0F) == 0; \
   dst--;                      \
   FLAG_Z = (dst) == 0;        \
   FLAG_N = true;

#define COMPARE(val)                     \
   byte v = (val);                       \
   FLAG_H = (cpu_a & 0x0F) < (v & 0x0F); \
   FLAG_C = cpu_a < v;                   \
   FLAG_N = true;                        \
   FLAG_Z = cpu_a == v;

#define CLEAR_FLAGS() \
   FLAG_C = false;    \
   FLAG_H = false;    \
   FLAG_Z = false;    \
   FLAG_N = false;

// Flag register
bool FLAG_C;
bool FLAG_H;
bool FLAG_Z;
bool FLAG_N;

// Helper operations

void cpu_none() {
   // Undefined opcode
   cpu_stopped = true;
}

void add16(byte* a_hi, byte* a_low, byte b_hi, byte b_low) {
   word a = (*a_hi << 8) | *a_low;
   word b = (b_hi << 8) | b_low;

   uint32_t carryCheck = a + b;

   FLAG_H = ((0x0FFF & a) + (0x0FFF & b) > 0x0FFF);
   FLAG_C = (carryCheck > 0x0000FFFF);
   FLAG_N = (false);

   a += b;
   *a_hi  = (a & 0xFF00) >> 8;
   *a_low = a & 0x00FF;
}

void inc16(byte* hi, byte* low) {
   if ((*low) == 0xFF) {
      (*hi)++;
   }
   (*low)++;
}

void dec16(byte* hi, byte* low) {
   if ((*low) == 0) {
      (*hi)--;
   }
   (*low)--;
}

void call() {
   byte hi, lo;
   TIME(2);
   lo = rbyte(cpu_pc);
   TIME(1);
   hi = rbyte(cpu_pc + 1);
   TIME(1);
   PUSHW(cpu_pc + 2);
   cpu_pc = (hi << 8) | lo;
}

void ret() {
   TIME(1);
   POPW(cpu_pc);
   TIME(1);
}

void jp() {
   byte hi, lo;
   TIME(2);
   lo = rbyte(cpu_pc);
   TIME(1);
   hi = rbyte(cpu_pc + 1);
   TIME(1);
   cpu_pc = (hi << 8) | lo;
}

void cpu_nop() { TIME(1); }

void rlc(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   FLAG_C = (t & 0x80);
   t      = (t << 1) | FLAG_C;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void rrc(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   FLAG_C = (t & 0x01);
   t      = (t >> 1) | (FLAG_C << 7);
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void rl(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   byte old_carry = FLAG_C;
   FLAG_C         = (t & 0x80);
   t              = (t << 1) | old_carry;
   FLAG_Z         = (t == 0);
   FLAG_N         = (false);
   FLAG_H         = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void rr(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   byte old_carry = FLAG_C;
   FLAG_C         = (t & 0x01);
   t              = (t >> 1) | (old_carry << 7);
   FLAG_Z         = (t == 0);
   FLAG_N         = (false);
   FLAG_H         = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void sla(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   FLAG_C = (t & 0x80);
   t      = t << 1;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void sra(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   byte msb = t & 0x80;
   FLAG_C   = (t & 0x01);
   t        = (t >> 1) | msb;
   FLAG_Z   = (t == 0);
   FLAG_N   = (false);
   FLAG_H   = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void swap(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   t = ((t << 4) | (t >> 4));
   CLEAR_FLAGS();
   FLAG_Z = (t == 0);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void srl(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   FLAG_C = (t & 0x01);
   t      = t >> 1;
   FLAG_Z = (t == 0);
   FLAG_N = (false);
   FLAG_H = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void bit(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   FLAG_Z = (!(t & (1 << bit)));
   FLAG_N = (false);
   FLAG_H = (true);
}

void res(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   t = t & ~(1 << bit);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

void set(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu_h, cpu_l);
   }

   t = t | (1 << bit);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu_h, cpu_l, t);
   } else {
      *inp = t;
   }
}

// LOAD / STORES

void cpu_lda_n() {
   TIME(2);
   LOAD(cpu_a, rbyte(cpu_pc++));
}
void cpu_ldb_n() {
   TIME(2);
   LOAD(cpu_b, rbyte(cpu_pc++));
}
void cpu_ldc_n() {
   TIME(2);
   LOAD(cpu_c, rbyte(cpu_pc++));
}
void cpu_ldd_n() {
   TIME(2);
   LOAD(cpu_d, rbyte(cpu_pc++));
}
void cpu_lde_n() {
   TIME(2);
   LOAD(cpu_e, rbyte(cpu_pc++));
}
void cpu_ldh_n() {
   TIME(2);
   LOAD(cpu_h, rbyte(cpu_pc++));
}
void cpu_ldl_n() {
   TIME(2);
   LOAD(cpu_l, rbyte(cpu_pc++));
}
void cpu_lda_a() {
   TIME(1);
   LOAD(cpu_a, cpu_a);
}
void cpu_lda_b() {
   TIME(1);
   LOAD(cpu_a, cpu_b);
}
void cpu_lda_c() {
   TIME(1);
   LOAD(cpu_a, cpu_c);
}
void cpu_lda_d() {
   TIME(1);
   LOAD(cpu_a, cpu_d);
}
void cpu_lda_e() {
   TIME(1);
   LOAD(cpu_a, cpu_e);
}
void cpu_lda_h() {
   TIME(1);
   LOAD(cpu_a, cpu_h);
}
void cpu_lda_l() {
   TIME(1);
   LOAD(cpu_a, cpu_l);
}
void cpu_ldb_a() {
   TIME(1);
   LOAD(cpu_b, cpu_a);
}
void cpu_ldb_b() {
   TIME(1);
   LOAD(cpu_b, cpu_b);
}
void cpu_ldb_c() {
   TIME(1);
   LOAD(cpu_b, cpu_c);
}
void cpu_ldb_d() {
   TIME(1);
   LOAD(cpu_b, cpu_d);
}
void cpu_ldb_e() {
   TIME(1);
   LOAD(cpu_b, cpu_e);
}
void cpu_ldb_h() {
   TIME(1);
   LOAD(cpu_b, cpu_h);
}
void cpu_ldb_l() {
   TIME(1);
   LOAD(cpu_b, cpu_l);
}
void cpu_ldc_a() {
   TIME(1);
   LOAD(cpu_c, cpu_a);
}
void cpu_ldc_b() {
   TIME(1);
   LOAD(cpu_c, cpu_b);
}
void cpu_ldc_c() {
   TIME(1);
   LOAD(cpu_c, cpu_c);
}
void cpu_ldc_d() {
   TIME(1);
   LOAD(cpu_c, cpu_d);
}
void cpu_ldc_e() {
   TIME(1);
   LOAD(cpu_c, cpu_e);
}
void cpu_ldc_h() {
   TIME(1);
   LOAD(cpu_c, cpu_h);
}
void cpu_ldc_l() {
   TIME(1);
   LOAD(cpu_c, cpu_l);
}
void cpu_ldd_a() {
   TIME(1);
   LOAD(cpu_d, cpu_a);
}
void cpu_ldd_b() {
   TIME(1);
   LOAD(cpu_d, cpu_b);
}
void cpu_ldd_c() {
   TIME(1);
   LOAD(cpu_d, cpu_c);
}
void cpu_ldd_d() {
   TIME(1);
   LOAD(cpu_d, cpu_d);
}
void cpu_ldd_e() {
   TIME(1);
   LOAD(cpu_d, cpu_e);
}
void cpu_ldd_h() {
   TIME(1);
   LOAD(cpu_d, cpu_h);
}
void cpu_ldd_l() {
   TIME(1);
   LOAD(cpu_d, cpu_l);
}
void cpu_lde_a() {
   TIME(1);
   LOAD(cpu_e, cpu_a);
}
void cpu_lde_b() {
   TIME(1);
   LOAD(cpu_e, cpu_b);
}
void cpu_lde_c() {
   TIME(1);
   LOAD(cpu_e, cpu_c);
}
void cpu_lde_d() {
   TIME(1);
   LOAD(cpu_e, cpu_d);
}
void cpu_lde_e() {
   TIME(1);
   LOAD(cpu_e, cpu_e);
}
void cpu_lde_h() {
   TIME(1);
   LOAD(cpu_e, cpu_h);
}
void cpu_lde_l() {
   TIME(1);
   LOAD(cpu_e, cpu_l);
}
void cpu_ldh_a() {
   TIME(1);
   LOAD(cpu_h, cpu_a);
}
void cpu_ldh_b() {
   TIME(1);
   LOAD(cpu_h, cpu_b);
}
void cpu_ldh_c() {
   TIME(1);
   LOAD(cpu_h, cpu_c);
}
void cpu_ldh_d() {
   TIME(1);
   LOAD(cpu_h, cpu_d);
}
void cpu_ldh_e() {
   TIME(1);
   LOAD(cpu_h, cpu_e);
}
void cpu_ldh_h() {
   TIME(1);
   LOAD(cpu_h, cpu_h);
}
void cpu_ldh_l() {
   TIME(1);
   LOAD(cpu_h, cpu_l);
}
void cpu_ldl_a() {
   TIME(1);
   LOAD(cpu_l, cpu_a);
}
void cpu_ldl_b() {
   TIME(1);
   LOAD(cpu_l, cpu_b);
}
void cpu_ldl_c() {
   TIME(1);
   LOAD(cpu_l, cpu_c);
}
void cpu_ldl_d() {
   TIME(1);
   LOAD(cpu_l, cpu_d);
}
void cpu_ldl_e() {
   TIME(1);
   LOAD(cpu_l, cpu_e);
}
void cpu_ldl_h() {
   TIME(1);
   LOAD(cpu_l, cpu_h);
}
void cpu_ldl_l() {
   TIME(1);
   LOAD(cpu_l, cpu_l);
}
void cpu_lda_at_hl() {
   TIME(2);
   LOAD(cpu_a, FETCH(cpu_h, cpu_l));
}
void cpu_ldb_at_hl() {
   TIME(2);
   LOAD(cpu_b, FETCH(cpu_h, cpu_l));
}
void cpu_ldc_at_hl() {
   TIME(2);
   LOAD(cpu_c, FETCH(cpu_h, cpu_l));
}
void cpu_ldd_at_hl() {
   TIME(2);
   LOAD(cpu_d, FETCH(cpu_h, cpu_l));
}
void cpu_lde_at_hl() {
   TIME(2);
   LOAD(cpu_e, FETCH(cpu_h, cpu_l));
}
void cpu_ldh_at_hl() {
   TIME(2);
   LOAD(cpu_h, FETCH(cpu_h, cpu_l));
}
void cpu_ldl_at_hl() {
   TIME(2);
   LOAD(cpu_l, FETCH(cpu_h, cpu_l));
}
void cpu_ld_at_hl_b() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_b);
}
void cpu_ld_at_hl_c() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_c);
}
void cpu_ld_at_hl_d() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_d);
}
void cpu_ld_at_hl_e() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_e);
}
void cpu_ld_at_hl_h() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_h);
}
void cpu_ld_at_hl_l() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_l);
}
void cpu_ld_at_hl_n() {
   TIME(3);
   STORE(cpu_h, cpu_l, rbyte(cpu_pc++));
}
void cpu_lda_at_bc() {
   TIME(2);
   LOAD(cpu_a, FETCH(cpu_b, cpu_c));
}
void cpu_lda_at_de() {
   TIME(2);
   LOAD(cpu_a, FETCH(cpu_d, cpu_e));
}
void cpu_ld_at_bc_a() {
   TIME(2);
   STORE(cpu_b, cpu_c, cpu_a);
}
void cpu_ld_at_de_a() {
   TIME(2);
   STORE(cpu_d, cpu_e, cpu_a);
}
void cpu_ld_at_hl_a() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_a);
}
void cpu_lda_at_c() {
   TIME(2);
   LOAD(cpu_a, rbyte(0xFF00 + cpu_c));
}
void cpu_ld_at_c_a() {
   TIME(2);
   STORE(0xFF, cpu_c, cpu_a);
}

void cpu_lda_at_nn() {
   TIME(4);
   LOAD(cpu_a, rbyte(rword(cpu_pc)));
   cpu_pc += 2;
}

void cpu_ld_at_nn_a() {
   TIME(4);
   wbyte(rword(cpu_pc), cpu_a);
   cpu_pc += 2;
}

void cpu_lda_at_hld() {
   TIME(2);
   cpu_a = FETCH(cpu_h, cpu_l);
   dec16(&cpu_h, &cpu_l);
}

void cpu_lda_at_hli() {
   TIME(2);
   cpu_a = FETCH(cpu_h, cpu_l);
   inc16(&cpu_h, &cpu_l);
}

void cpu_ld_at_hld_a() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_a);
   dec16(&cpu_h, &cpu_l);
}

void cpu_ld_at_hli_a() {
   TIME(2);
   STORE(cpu_h, cpu_l, cpu_a);
   inc16(&cpu_h, &cpu_l);
}

void cpu_ld_n_a() {
   TIME(3);
   STORE(0xFF, rbyte(cpu_pc++), cpu_a);
}

void cpu_ld_a_n() {
   TIME(3);
   cpu_a = FETCH(0xFF, rbyte(cpu_pc++));
}

void cpu_ldbc_nn() {
   TIME(3);
   cpu_c = rbyte(cpu_pc++);
   cpu_b = rbyte(cpu_pc++);
}

void cpu_ldde_nn() {
   TIME(3);
   cpu_e = rbyte(cpu_pc++);
   cpu_d = rbyte(cpu_pc++);
}

void cpu_ldhl_nn() {
   TIME(3);
   cpu_l = rbyte(cpu_pc++);
   cpu_h = rbyte(cpu_pc++);
}

void cpu_ldsp_nn() {
   TIME(3);
   cpu_sp = rword(cpu_pc);
   cpu_pc += 2;
}

void cpu_ldsp_hl() {
   TIME(2);
   cpu_sp = (cpu_h << 8) | cpu_l;
}

void cpu_ldhl_sp_n() {
   TIME(2);
   byte next = rbyte(cpu_pc++);
   TIME(1);
   sbyte off = (sbyte)next;
   int res   = off + cpu_sp;
   CLEAR_FLAGS();
   FLAG_C = ((cpu_sp & 0xFF) + next > 0xFF);
   FLAG_H = ((cpu_sp & 0xF) + (next & 0xF) > 0xF);
   cpu_h  = (res & 0xFF00) >> 8;
   cpu_l  = res & 0x00FF;
}

void cpu_ld_nn_sp() {
   TIME(5);
   word tmp = rword(cpu_pc);
   cpu_pc += 2;
   wword(tmp, cpu_sp);
}

// PUSH / POP

void cpu_pushbc() {
   TIME(2);
   PUSH(cpu_b, cpu_c);
}
void cpu_pushde() {
   TIME(2);
   PUSH(cpu_d, cpu_e);
}
void cpu_pushhl() {
   TIME(2);
   PUSH(cpu_h, cpu_l);
}
void cpu_popbc() {
   TIME(1);
   POP(cpu_b, cpu_c);
}
void cpu_popde() {
   TIME(1);
   POP(cpu_d, cpu_e);
}
void cpu_pophl() {
   TIME(1);
   POP(cpu_h, cpu_l);
}

void cpu_pushaf() {
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
   TIME(2);
   PUSH(cpu_a, flags);
}

void cpu_popaf() {
   byte flags = 0;
   TIME(1);
   POP(cpu_a, flags);
   FLAG_Z = (flags & BITMASK_Z);
   FLAG_C = (flags & BITMASK_C);
   FLAG_H = (flags & BITMASK_H);
   FLAG_N = (flags & BITMASK_N);
}

// AND / OR / XOR
void cpu_and_a() {
   TIME(1);
   AND(cpu_a);
}
void cpu_and_b() {
   TIME(1);
   AND(cpu_b);
}
void cpu_and_c() {
   TIME(1);
   AND(cpu_c);
}
void cpu_and_d() {
   TIME(1);
   AND(cpu_d);
}
void cpu_and_e() {
   TIME(1);
   AND(cpu_e);
}
void cpu_and_h() {
   TIME(1);
   AND(cpu_h);
}
void cpu_and_l() {
   TIME(1);
   AND(cpu_l);
}
void cpu_or_a() {
   TIME(1);
   OR(cpu_a);
}
void cpu_or_b() {
   TIME(1);
   OR(cpu_b);
}
void cpu_or_c() {
   TIME(1);
   OR(cpu_c);
}
void cpu_or_d() {
   TIME(1);
   OR(cpu_d);
}
void cpu_or_e() {
   TIME(1);
   OR(cpu_e);
}
void cpu_or_h() {
   TIME(1);
   OR(cpu_h);
}
void cpu_or_l() {
   TIME(1);
   OR(cpu_l);
}
void cpu_xor_a() {
   TIME(1);
   XOR(cpu_a);
}
void cpu_xor_b() {
   TIME(1);
   XOR(cpu_b);
}
void cpu_xor_c() {
   TIME(1);
   XOR(cpu_c);
}
void cpu_xor_d() {
   TIME(1);
   XOR(cpu_d);
}
void cpu_xor_e() {
   TIME(1);
   XOR(cpu_e);
}
void cpu_xor_h() {
   TIME(1);
   XOR(cpu_h);
}
void cpu_xor_l() {
   TIME(1);
   XOR(cpu_l);
}
void cpu_and_at_hl() {
   TIME(2);
   AND(FETCH(cpu_h, cpu_l));
}
void cpu_or_at_hl() {
   TIME(2);
   OR(FETCH(cpu_h, cpu_l));
}
void cpu_xor_at_hl() {
   TIME(2);
   XOR(FETCH(cpu_h, cpu_l));
}
void cpu_and_n() {
   TIME(2);
   AND(rbyte(cpu_pc++));
}
void cpu_or_n() {
   TIME(2);
   OR(rbyte(cpu_pc++));
}
void cpu_xor_n() {
   TIME(2);
   XOR(rbyte(cpu_pc++));
}

void cpu_cb() {
   TIME(2);
   byte sub_op  = rbyte(cpu_pc++);
   byte* regs[] = {
         &cpu_b, &cpu_c, &cpu_d, &cpu_e, &cpu_h, &cpu_l, NULL, &cpu_a};
   byte index = sub_op & 0x0F;
   if (index == 6 || index == 14) {
      TIME(1);
   }
   if (index >= 8) {
      index -= 8;
      switch (sub_op & 0xF0) {
         case 0x00: rrc(regs[index]); break;
         case 0x10: rr(regs[index]); break;
         case 0x20: sra(regs[index]); break;
         case 0x30: srl(regs[index]); break;
         case 0x40: bit(regs[index], 1); break;
         case 0x50: bit(regs[index], 3); break;
         case 0x60: bit(regs[index], 5); break;
         case 0x70: bit(regs[index], 7); break;
         case 0x80: res(regs[index], 1); break;
         case 0x90: res(regs[index], 3); break;
         case 0xA0: res(regs[index], 5); break;
         case 0xB0: res(regs[index], 7); break;
         case 0xC0: set(regs[index], 1); break;
         case 0xD0: set(regs[index], 3); break;
         case 0xE0: set(regs[index], 5); break;
         case 0xF0: set(regs[index], 7); break;
         default: cpu_none(); break;
      }

   } else {
      switch (sub_op & 0xF0) {
         case 0x00: rlc(regs[index]); break;
         case 0x10: rl(regs[index]); break;
         case 0x20: sla(regs[index]); break;
         case 0x30: swap(regs[index]); break;
         case 0x40: bit(regs[index], 0); break;
         case 0x50: bit(regs[index], 2); break;
         case 0x60: bit(regs[index], 4); break;
         case 0x70: bit(regs[index], 6); break;
         case 0x80: res(regs[index], 0); break;
         case 0x90: res(regs[index], 2); break;
         case 0xA0: res(regs[index], 4); break;
         case 0xB0: res(regs[index], 6); break;
         case 0xC0: set(regs[index], 0); break;
         case 0xD0: set(regs[index], 2); break;
         case 0xE0: set(regs[index], 4); break;
         case 0xF0: set(regs[index], 6); break;
         default: cpu_none(); break;
      }
   }
}

// JUMP / RETURN

void cpu_jp_nn() { jp(); }

void cpu_jp_nz_nn() {
   if (!FLAG_Z) {
      jp();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_jp_z_nn() {
   if (FLAG_Z) {
      jp();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_jp_nc_nn() {
   if (!FLAG_C) {
      jp();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_jp_c_nn() {
   if (FLAG_C) {
      jp();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_jp_at_hl() {
   TIME(1);
   cpu_pc = ((cpu_h << 8) | cpu_l);
}

void cpu_jr_n() {
   TIME(3);
   JR();
}

void cpu_jr_nz_n() {
   if (!FLAG_Z) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu_pc++;
   }
}

void cpu_jr_z_n() {
   if (FLAG_Z) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu_pc++;
   }
}

void cpu_jr_nc_n() {
   if (!FLAG_C) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu_pc++;
   }
}

void cpu_jr_c_n() {
   if (FLAG_C) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu_pc++;
   }
}

void cpu_call_nn() { call(); }

void cpu_call_nz_nn() {
   if (!FLAG_Z) {
      call();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_call_z_nn() {
   if (FLAG_Z) {
      call();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_call_nc_nn() {
   if (!FLAG_C) {
      call();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

void cpu_call_c_nn() {
   if (FLAG_C) {
      call();
   } else {
      TIME(3);
      cpu_pc += 2;
   }
}

// Restarts

void cpu_rst_00h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x00;
}

void cpu_rst_08h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x08;
}

void cpu_rst_10h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x10;
}

void cpu_rst_18h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x18;
}

void cpu_rst_20h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x20;
}

void cpu_rst_28h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x28;
}

void cpu_rst_30h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x30;
}

void cpu_rst_38h() {
   TIME(2);
   PUSHW(cpu_pc);
   cpu_halted  = false;
   cpu_stopped = false;
   cpu_pc      = 0x38;
}

// Returns

void cpu_ret() { ret(); }

void cpu_ret_nz() {
   if (!FLAG_Z) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_ret_z() {
   if (FLAG_Z) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_ret_nc() {
   if (!FLAG_C) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_ret_c() {
   if (FLAG_C) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_reti() {
   cpu_ime       = true;
   cpu_ime_delay = false;
   ret();
}

// 16 bit math

void cpu_add16_hl_bc() {
   TIME(2);
   add16(&cpu_h, &cpu_l, cpu_b, cpu_c);
}

void cpu_add16_hl_de() {
   TIME(2);
   add16(&cpu_h, &cpu_l, cpu_d, cpu_e);
}

void cpu_add16_hl_hl() {
   TIME(2);
   add16(&cpu_h, &cpu_l, cpu_h, cpu_l);
}

void cpu_add16_hl_sp() {
   TIME(2);
   add16(&cpu_h, &cpu_l, (cpu_sp & 0xFF00) >> 8, cpu_sp & 0x00FF);
}

void cpu_add16_sp_n() {
   TIME(2);
   byte val = rbyte(cpu_pc++);
   TIME(2);
   sbyte off = (sbyte)val;
   CLEAR_FLAGS();
   FLAG_H = ((cpu_sp & 0xF) + (val & 0xF) > 0xF);
   FLAG_C = (((cpu_sp & 0xFF) + val > 0xFF));
   cpu_sp += off;
}

// 0x03
void cpu_inc16_bc() {
   TIME(2);
   inc16(&cpu_b, &cpu_c);
}

void cpu_inc16_de() {
   TIME(2);
   inc16(&cpu_d, &cpu_e);
}

void cpu_inc16_hl() {
   TIME(2);
   inc16(&cpu_h, &cpu_l);
}

void cpu_inc16_sp() {
   TIME(2);
   cpu_sp++;
}

void cpu_dec16_bc() {
   TIME(2);
   dec16(&cpu_b, &cpu_c);
}

void cpu_dec16_de() {
   TIME(2);
   dec16(&cpu_d, &cpu_e);
}

void cpu_dec16_hl() {
   TIME(2);
   dec16(&cpu_h, &cpu_l);
}

void cpu_dec16_sp() {
   TIME(2);
   cpu_sp--;
}

// Rotates / misc

void cpu_rla() {
   TIME(1);
   rl(&cpu_a);
   FLAG_Z = false;
}

void cpu_rlca() {
   TIME(1);
   rlc(&cpu_a);
   FLAG_Z = false;
}

void cpu_rrca() {
   TIME(1);
   rrc(&cpu_a);
   FLAG_Z = false;
}

void cpu_rra() {
   TIME(1);
   rr(&cpu_a);
   FLAG_Z = false;
}

void cpu_di() {
   //   //printf("[%04X] DI, %02X, %d, %lld\n", cpu_pc - 1,
   //   rbyte(LCD_STATUS_ADDR), ppu_ly, ppu_get_timer());
   TIME(1);
   cpu_ime       = false;
   cpu_ime_delay = false;
}

void cpu_ei() {
   //   //printf("[%04X] EI, %02X, %d, %lld\n", cpu_pc - 1,
   //   rbyte(LCD_STATUS_ADDR), ppu_ly, ppu_get_timer());
   TIME(1);
   cpu_ime       = true;
   cpu_ime_delay = true;
}

// ADD / ADC / SUB / SUBC
void cpu_add_a_a() {
   TIME(1);
   ADD(cpu_a);
}
void cpu_add_a_b() {
   TIME(1);
   ADD(cpu_b);
}
void cpu_add_a_c() {
   TIME(1);
   ADD(cpu_c);
}
void cpu_add_a_d() {
   TIME(1);
   ADD(cpu_d);
}
void cpu_add_a_e() {
   TIME(1);
   ADD(cpu_e);
}
void cpu_add_a_h() {
   TIME(1);
   ADD(cpu_h);
}
void cpu_add_a_l() {
   TIME(1);
   ADD(cpu_l);
}
void cpu_adc_a_a() {
   TIME(1);
   ADC(cpu_a);
}
void cpu_adc_a_b() {
   TIME(1);
   ADC(cpu_b);
}
void cpu_adc_a_c() {
   TIME(1);
   ADC(cpu_c);
}
void cpu_adc_a_d() {
   TIME(1);
   ADC(cpu_d);
}
void cpu_adc_a_e() {
   TIME(1);
   ADC(cpu_e);
}
void cpu_adc_a_h() {
   TIME(1);
   ADC(cpu_h);
}
void cpu_adc_a_l() {
   TIME(1);
   ADC(cpu_l);
}
void cpu_sub_a_a() {
   TIME(1);
   SUB(cpu_a);
}
void cpu_sub_a_b() {
   TIME(1);
   SUB(cpu_b);
}
void cpu_sub_a_c() {
   TIME(1);
   SUB(cpu_c);
}
void cpu_sub_a_d() {
   TIME(1);
   SUB(cpu_d);
}
void cpu_sub_a_e() {
   TIME(1);
   SUB(cpu_e);
}
void cpu_sub_a_h() {
   TIME(1);
   SUB(cpu_h);
}
void cpu_sub_a_l() {
   TIME(1);
   SUB(cpu_l);
}
void cpu_sbc_a_a() {
   TIME(1);
   SBC(cpu_a);
}
void cpu_sbc_a_b() {
   TIME(1);
   SBC(cpu_b);
}
void cpu_sbc_a_c() {
   TIME(1);
   SBC(cpu_c);
}
void cpu_sbc_a_d() {
   TIME(1);
   SBC(cpu_d);
}
void cpu_sbc_a_e() {
   TIME(1);
   SBC(cpu_e);
}
void cpu_sbc_a_h() {
   TIME(1);
   SBC(cpu_h);
}
void cpu_sbc_a_l() {
   TIME(1);
   SBC(cpu_l);
}
void cpu_add_a_n() {
   TIME(2);
   ADD(rbyte(cpu_pc++));
}
void cpu_adc_a_n() {
   TIME(2);
   ADC(rbyte(cpu_pc++));
}
void cpu_sub_a_n() {
   TIME(2);
   SUB(rbyte(cpu_pc++));
}
void cpu_sbc_a_n() {
   TIME(2);
   SBC(rbyte(cpu_pc++));
}
void cpu_add_a_at_hl() {
   TIME(2);
   ADD(FETCH(cpu_h, cpu_l));
}
void cpu_adc_a_at_hl() {
   TIME(2);
   ADC(FETCH(cpu_h, cpu_l));
}
void cpu_sub_a_at_hl() {
   TIME(2);
   SUB(FETCH(cpu_h, cpu_l));
}
void cpu_sbc_a_at_hl() {
   TIME(2);
   SBC(FETCH(cpu_h, cpu_l));
}

// INCREMENT / DECREMENT
void cpu_inc_a() {
   INC(cpu_a);
   TIME(1);
}
void cpu_inc_b() {
   INC(cpu_b);
   TIME(1);
}
void cpu_inc_c() {
   INC(cpu_c);
   TIME(1);
}
void cpu_inc_d() {
   INC(cpu_d);
   TIME(1);
}
void cpu_inc_e() {
   INC(cpu_e);
   TIME(1);
}
void cpu_inc_h() {
   TIME(1);
   INC(cpu_h);
}
void cpu_inc_l() {
   INC(cpu_l);
   TIME(1);
}
void cpu_dec_a() {
   DEC(cpu_a);
   TIME(1);
}
void cpu_dec_b() {
   DEC(cpu_b);
   TIME(1);
}
void cpu_dec_c() {
   DEC(cpu_c);
   TIME(1);
}
void cpu_dec_d() {
   DEC(cpu_d);
   TIME(1);
}
void cpu_dec_e() {
   DEC(cpu_e);
   TIME(1);
}
void cpu_dec_h() {
   DEC(cpu_h);
   TIME(1);
}
void cpu_dec_l() {
   DEC(cpu_l);
   TIME(1);
}

void cpu_inc_at_hl() {
   TIME(2);
   byte val = FETCH(cpu_h, cpu_l);
   FLAG_H   = (val & 0x0F) + 1 > 0x0F;
   FLAG_N   = false;
   val++;
   FLAG_Z = val == 0;
   TIME(1);
   STORE(cpu_h, cpu_l, val);
}

void cpu_dec_at_hl() {
   TIME(2);
   byte val = FETCH(cpu_h, cpu_l);
   FLAG_H   = (val & 0x0F) == 0;
   val--;
   FLAG_Z = val == 0;
   FLAG_N = true;
   TIME(1);
   STORE(cpu_h, cpu_l, val);
}

// COMPARE
void cpu_cp_a() {
   TIME(1);
   COMPARE(cpu_a);
}
void cpu_cp_b() {
   TIME(1);
   COMPARE(cpu_b);
}
void cpu_cp_c() {
   TIME(1);
   COMPARE(cpu_c);
}
void cpu_cp_d() {
   TIME(1);
   COMPARE(cpu_d);
}
void cpu_cp_e() {
   TIME(1);
   COMPARE(cpu_e);
}
void cpu_cp_h() {
   TIME(1);
   COMPARE(cpu_h);
}
void cpu_cp_l() {
   TIME(1);
   COMPARE(cpu_l);
}
void cpu_cp_at_hl() {
   TIME(2);
   COMPARE(FETCH(cpu_h, cpu_l));
}
void cpu_cp_a_n() {
   TIME(2);
   COMPARE(rbyte(cpu_pc++));
}

void cpu_cpl() {
   TIME(1);
   cpu_a  = ~cpu_a;
   FLAG_H = true;
   FLAG_N = true;
}

void cpu_ccf() {
   TIME(1);
   FLAG_C = !FLAG_C;
   FLAG_N = false;
   FLAG_H = false;
}

void cpu_scf() {
   TIME(1);
   FLAG_H = false;
   FLAG_N = false;
   FLAG_C = true;
}

void cpu_halt() {
   TIME(1);
   cpu_halted = true;
}

void cpu_stop() {
   TIME(1);
   cpu_stopped = true;
}

// DAA implementation based on code found at
// http://forums.nesdev.com/viewtopic.php?t=9088
void cpu_daa() {
   TIME(1);
   int a = cpu_a;
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
   cpu_a  = (byte)a;
   FLAG_H = false;
   FLAG_Z = !cpu_a;
   if ((a & 0x100) == 0x100) {
      FLAG_C = true;
   }
}

#endif
