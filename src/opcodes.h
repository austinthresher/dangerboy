// Opcode macros
#ifndef __OPCODES_H__
#define __OPCODES_H__

#define TIME(x) cpu_advance_time((x)<<2)

#define LOAD(reg, val) reg = (val);

#define STORE(hi, lo, val) wbyte((((hi)&0xFF) << 8) | ((lo)&0xFF), (val));

#define FETCH(hi, lo) rbyte((((hi)&0xFF) << 8) | ((lo)&0xFF))

#define AND(val)             \
   cpu.a  = cpu.a & (val);   \
   cpu.cf = cpu.nf = false;  \
   cpu.zf          = !cpu.a; \
   cpu.hf          = true;

#define OR(val)                      \
   cpu.a  = cpu.a | (val);           \
   cpu.cf = cpu.hf = cpu.nf = false; \
   cpu.zf                   = !cpu.a;

#define XOR(val)                     \
   cpu.a  = cpu.a ^ (val);           \
   cpu.cf = cpu.hf = cpu.nf = false; \
   cpu.zf                   = !cpu.a;

#define PUSH(hi, lo)      \
   TIME(1);               \
   wbyte(--cpu.sp, (hi)); \
   TIME(1);               \
   wbyte(--cpu.sp, (lo));

#define POP(hi, lo)      \
   TIME(1);              \
   lo = rbyte(cpu.sp++); \
   TIME(1);              \
   hi = rbyte(cpu.sp++);

#define PUSHW(val)                       \
   TIME(1);                              \
   wbyte(--cpu.sp, ((val) >> 8) & 0xFF); \
   TIME(1);                              \
   wbyte(--cpu.sp, (val)&0xFF);

#define POPW(val)          \
   TIME(1);                \
   val &= 0xFF00;          \
   val |= rbyte(cpu.sp++); \
   TIME(1);                \
   val &= 0x00FF;          \
   val |= rbyte(cpu.sp++) << 8;

#define JR() cpu.pc += (sbyte)rbyte(cpu.pc) + 1;

#define ADD(val)                                    \
   byte v   = (val);                                \
   cpu.nf   = false;                                \
   cpu.hf   = ((cpu.a & 0x0F) + (v & 0x0F)) > 0x0F; \
   word tmp = cpu.a + v;                            \
   cpu.a    = tmp & 0xFF;                           \
   cpu.cf   = tmp > 0xFF;                           \
   cpu.zf   = cpu.a == 0;

#define ADC(val)                                           \
   byte v   = (val);                                       \
   cpu.nf   = false;                                       \
   word tmp = cpu.a + v + cpu.cf;                          \
   cpu.hf   = (cpu.a & 0x0F) + (v & 0x0F) + cpu.cf > 0x0F; \
   cpu.cf   = tmp > 0xFF;                                  \
   cpu.a    = tmp & 0xFF;                                  \
   cpu.zf   = cpu.a == 0;

#define SUB(val)                         \
   byte v = (val);                       \
   cpu.nf = true;                        \
   cpu.hf = (cpu.a & 0x0F) < (v & 0x0F); \
   cpu.cf = cpu.a < v;                   \
   cpu.a -= v;                           \
   cpu.zf = cpu.a == 0;

#define SBC(val)                                    \
   byte v   = (val);                                \
   cpu.nf   = true;                                 \
   byte tmp = cpu.a - v - cpu.cf;                   \
   cpu.hf   = (cpu.a & 0x0F) < (v & 0x0F) + cpu.cf; \
   cpu.cf   = cpu.a < v + cpu.cf;                   \
   cpu.a    = tmp;                                  \
   cpu.zf   = cpu.a == 0;

#define INC(dst)                     \
   cpu.hf = ((dst)&0x0F) + 1 > 0x0F; \
   cpu.nf = false;                   \
   dst++;                            \
   cpu.zf = (dst) == 0;

#define DEC(dst)               \
   cpu.hf = ((dst)&0x0F) == 0; \
   dst--;                      \
   cpu.zf = (dst) == 0;        \
   cpu.nf = true;

#define COMPARE(val)                     \
   byte v = (val);                       \
   cpu.hf = (cpu.a & 0x0F) < (v & 0x0F); \
   cpu.cf = cpu.a < v;                   \
   cpu.nf = true;                        \
   cpu.zf = cpu.a == v;

#define CLEAR_FLAGS() \
   cpu.cf = false;    \
   cpu.hf = false;    \
   cpu.zf = false;    \
   cpu.nf = false;

// Helper operations

void cpu_none() {
   // Undefined opcode
   cpu.stopped = true;
}

void add16(byte* a_hi, byte* a_low, byte b_hi, byte b_low) {
   word a = (*a_hi << 8) | *a_low;
   word b = (b_hi << 8) | b_low;

   uint32_t carryCheck = a + b;

   cpu.hf = ((0x0FFF & a) + (0x0FFF & b) > 0x0FFF);
   cpu.cf = (carryCheck > 0x0000FFFF);
   cpu.nf = (false);

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
   lo = rbyte(cpu.pc);
   TIME(1);
   hi = rbyte(cpu.pc + 1);
   TIME(1);
   PUSHW(cpu.pc + 2);
   cpu.pc = (hi << 8) | lo;
}

void ret() {
   TIME(1);
   POPW(cpu.pc);
   TIME(1);
}

void jp() {
   byte hi, lo;
   TIME(2);
   lo = rbyte(cpu.pc);
   TIME(1);
   hi = rbyte(cpu.pc + 1);
   TIME(1);
   cpu.pc = (hi << 8) | lo;
}

void cpu_nop() { TIME(1); }

void rlc(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   cpu.cf = (t & 0x80);
   t      = (t << 1) | cpu.cf;
   cpu.zf = (t == 0);
   cpu.nf = (false);
   cpu.hf = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void rrc(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   cpu.cf = (t & 0x01);
   t      = (t >> 1) | (cpu.cf << 7);
   cpu.zf = (t == 0);
   cpu.nf = (false);
   cpu.hf = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void rl(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   byte old_carry = cpu.cf;
   cpu.cf         = (t & 0x80);
   t              = (t << 1) | old_carry;
   cpu.zf         = (t == 0);
   cpu.nf         = (false);
   cpu.hf         = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void rr(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   byte old_carry = cpu.cf;
   cpu.cf         = (t & 0x01);
   t              = (t >> 1) | (old_carry << 7);
   cpu.zf         = (t == 0);
   cpu.nf         = (false);
   cpu.hf         = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void sla(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   cpu.cf = (t & 0x80);
   t      = t << 1;
   cpu.zf = (t == 0);
   cpu.nf = (false);
   cpu.hf = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void sra(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   byte msb = t & 0x80;
   cpu.cf   = (t & 0x01);
   t        = (t >> 1) | msb;
   cpu.zf   = (t == 0);
   cpu.nf   = (false);
   cpu.hf   = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void swap(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   t = ((t << 4) | (t >> 4));
   CLEAR_FLAGS();
   cpu.zf = (t == 0);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void srl(byte* inp) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   cpu.cf = (t & 0x01);
   t      = t >> 1;
   cpu.zf = (t == 0);
   cpu.nf = (false);
   cpu.hf = (false);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void bit(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   cpu.zf = (!(t & (1 << bit)));
   cpu.nf = (false);
   cpu.hf = (true);
}

void res(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   t = t & ~(1 << bit);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

void set(byte* inp, byte bit) {
   byte t;
   if (inp != NULL) {
      t = *inp;
   } else {
      t = FETCH(cpu.h, cpu.l);
   }

   t = t | (1 << bit);

   if (inp == NULL) {
      TIME(1);
      STORE(cpu.h, cpu.l, t);
   } else {
      *inp = t;
   }
}

// LOAD / STORES

void cpu_lda_n() {
   TIME(2);
   LOAD(cpu.a, rbyte(cpu.pc++));
}
void cpu_ldb_n() {
   TIME(2);
   LOAD(cpu.b, rbyte(cpu.pc++));
}
void cpu_ldc_n() {
   TIME(2);
   LOAD(cpu.c, rbyte(cpu.pc++));
}
void cpu_ldd_n() {
   TIME(2);
   LOAD(cpu.d, rbyte(cpu.pc++));
}
void cpu_lde_n() {
   TIME(2);
   LOAD(cpu.e, rbyte(cpu.pc++));
}
void cpu_ldh_n() {
   TIME(2);
   LOAD(cpu.h, rbyte(cpu.pc++));
}
void cpu_ldl_n() {
   TIME(2);
   LOAD(cpu.l, rbyte(cpu.pc++));
}
void cpu_lda_a() {
   TIME(1);
   LOAD(cpu.a, cpu.a);
}
void cpu_lda_b() {
   TIME(1);
   LOAD(cpu.a, cpu.b);
}
void cpu_lda_c() {
   TIME(1);
   LOAD(cpu.a, cpu.c);
}
void cpu_lda_d() {
   TIME(1);
   LOAD(cpu.a, cpu.d);
}
void cpu_lda_e() {
   TIME(1);
   LOAD(cpu.a, cpu.e);
}
void cpu_lda_h() {
   TIME(1);
   LOAD(cpu.a, cpu.h);
}
void cpu_lda_l() {
   TIME(1);
   LOAD(cpu.a, cpu.l);
}
void cpu_ldb_a() {
   TIME(1);
   LOAD(cpu.b, cpu.a);
}
void cpu_ldb_b() {
   TIME(1);
   LOAD(cpu.b, cpu.b);
}
void cpu_ldb_c() {
   TIME(1);
   LOAD(cpu.b, cpu.c);
}
void cpu_ldb_d() {
   TIME(1);
   LOAD(cpu.b, cpu.d);
}
void cpu_ldb_e() {
   TIME(1);
   LOAD(cpu.b, cpu.e);
}
void cpu_ldb_h() {
   TIME(1);
   LOAD(cpu.b, cpu.h);
}
void cpu_ldb_l() {
   TIME(1);
   LOAD(cpu.b, cpu.l);
}
void cpu_ldc_a() {
   TIME(1);
   LOAD(cpu.c, cpu.a);
}
void cpu_ldc_b() {
   TIME(1);
   LOAD(cpu.c, cpu.b);
}
void cpu_ldc_c() {
   TIME(1);
   LOAD(cpu.c, cpu.c);
}
void cpu_ldc_d() {
   TIME(1);
   LOAD(cpu.c, cpu.d);
}
void cpu_ldc_e() {
   TIME(1);
   LOAD(cpu.c, cpu.e);
}
void cpu_ldc_h() {
   TIME(1);
   LOAD(cpu.c, cpu.h);
}
void cpu_ldc_l() {
   TIME(1);
   LOAD(cpu.c, cpu.l);
}
void cpu_ldd_a() {
   TIME(1);
   LOAD(cpu.d, cpu.a);
}
void cpu_ldd_b() {
   TIME(1);
   LOAD(cpu.d, cpu.b);
}
void cpu_ldd_c() {
   TIME(1);
   LOAD(cpu.d, cpu.c);
}
void cpu_ldd_d() {
   TIME(1);
   LOAD(cpu.d, cpu.d);
}
void cpu_ldd_e() {
   TIME(1);
   LOAD(cpu.d, cpu.e);
}
void cpu_ldd_h() {
   TIME(1);
   LOAD(cpu.d, cpu.h);
}
void cpu_ldd_l() {
   TIME(1);
   LOAD(cpu.d, cpu.l);
}
void cpu_lde_a() {
   TIME(1);
   LOAD(cpu.e, cpu.a);
}
void cpu_lde_b() {
   TIME(1);
   LOAD(cpu.e, cpu.b);
}
void cpu_lde_c() {
   TIME(1);
   LOAD(cpu.e, cpu.c);
}
void cpu_lde_d() {
   TIME(1);
   LOAD(cpu.e, cpu.d);
}
void cpu_lde_e() {
   TIME(1);
   LOAD(cpu.e, cpu.e);
}
void cpu_lde_h() {
   TIME(1);
   LOAD(cpu.e, cpu.h);
}
void cpu_lde_l() {
   TIME(1);
   LOAD(cpu.e, cpu.l);
}
void cpu_ldh_a() {
   TIME(1);
   LOAD(cpu.h, cpu.a);
}
void cpu_ldh_b() {
   TIME(1);
   LOAD(cpu.h, cpu.b);
}
void cpu_ldh_c() {
   TIME(1);
   LOAD(cpu.h, cpu.c);
}
void cpu_ldh_d() {
   TIME(1);
   LOAD(cpu.h, cpu.d);
}
void cpu_ldh_e() {
   TIME(1);
   LOAD(cpu.h, cpu.e);
}
void cpu_ldh_h() {
   TIME(1);
   LOAD(cpu.h, cpu.h);
}
void cpu_ldh_l() {
   TIME(1);
   LOAD(cpu.h, cpu.l);
}
void cpu_ldl_a() {
   TIME(1);
   LOAD(cpu.l, cpu.a);
}
void cpu_ldl_b() {
   TIME(1);
   LOAD(cpu.l, cpu.b);
}
void cpu_ldl_c() {
   TIME(1);
   LOAD(cpu.l, cpu.c);
}
void cpu_ldl_d() {
   TIME(1);
   LOAD(cpu.l, cpu.d);
}
void cpu_ldl_e() {
   TIME(1);
   LOAD(cpu.l, cpu.e);
}
void cpu_ldl_h() {
   TIME(1);
   LOAD(cpu.l, cpu.h);
}
void cpu_ldl_l() {
   TIME(1);
   LOAD(cpu.l, cpu.l);
}
void cpu_lda_at_hl() {
   TIME(2);
   LOAD(cpu.a, FETCH(cpu.h, cpu.l));
}
void cpu_ldb_at_hl() {
   TIME(2);
   LOAD(cpu.b, FETCH(cpu.h, cpu.l));
}
void cpu_ldc_at_hl() {
   TIME(2);
   LOAD(cpu.c, FETCH(cpu.h, cpu.l));
}
void cpu_ldd_at_hl() {
   TIME(2);
   LOAD(cpu.d, FETCH(cpu.h, cpu.l));
}
void cpu_lde_at_hl() {
   TIME(2);
   LOAD(cpu.e, FETCH(cpu.h, cpu.l));
}
void cpu_ldh_at_hl() {
   TIME(2);
   LOAD(cpu.h, FETCH(cpu.h, cpu.l));
}
void cpu_ldl_at_hl() {
   TIME(2);
   LOAD(cpu.l, FETCH(cpu.h, cpu.l));
}
void cpu_ld_at_hl_b() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.b);
}
void cpu_ld_at_hl_c() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.c);
}
void cpu_ld_at_hl_d() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.d);
}
void cpu_ld_at_hl_e() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.e);
}
void cpu_ld_at_hl_h() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.h);
}
void cpu_ld_at_hl_l() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.l);
}
void cpu_ld_at_hl_n() {
   TIME(3);
   STORE(cpu.h, cpu.l, rbyte(cpu.pc++));
}
void cpu_lda_at_bc() {
   TIME(2);
   LOAD(cpu.a, FETCH(cpu.b, cpu.c));
}
void cpu_lda_at_de() {
   TIME(2);
   LOAD(cpu.a, FETCH(cpu.d, cpu.e));
}
void cpu_ld_at_bc_a() {
   TIME(2);
   STORE(cpu.b, cpu.c, cpu.a);
}
void cpu_ld_at_de_a() {
   TIME(2);
   STORE(cpu.d, cpu.e, cpu.a);
}
void cpu_ld_at_hl_a() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.a);
}
void cpu_lda_at_c() {
   TIME(2);
   LOAD(cpu.a, rbyte(0xFF00 + cpu.c));
}
void cpu_ld_at_c_a() {
   TIME(2);
   STORE(0xFF, cpu.c, cpu.a);
}

void cpu_lda_at_nn() {
   TIME(4);
   LOAD(cpu.a, rbyte(rword(cpu.pc)));
   cpu.pc += 2;
}

void cpu_ld_at_nn_a() {
   TIME(4);
   wbyte(rword(cpu.pc), cpu.a);
   cpu.pc += 2;
}

void cpu_lda_at_hld() {
   TIME(2);
   cpu.a = FETCH(cpu.h, cpu.l);
   dec16(&cpu.h, &cpu.l);
}

void cpu_lda_at_hli() {
   TIME(2);
   cpu.a = FETCH(cpu.h, cpu.l);
   inc16(&cpu.h, &cpu.l);
}

void cpu_ld_at_hld_a() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.a);
   dec16(&cpu.h, &cpu.l);
}

void cpu_ld_at_hli_a() {
   TIME(2);
   STORE(cpu.h, cpu.l, cpu.a);
   inc16(&cpu.h, &cpu.l);
}

void cpu_ld_n_a() {
   TIME(3);
   STORE(0xFF, rbyte(cpu.pc++), cpu.a);
}

void cpu_ld_a_n() {
   TIME(3);
   cpu.a = FETCH(0xFF, rbyte(cpu.pc++));
}

void cpu_ldbc_nn() {
   TIME(3);
   cpu.c = rbyte(cpu.pc++);
   cpu.b = rbyte(cpu.pc++);
}

void cpu_ldde_nn() {
   TIME(3);
   cpu.e = rbyte(cpu.pc++);
   cpu.d = rbyte(cpu.pc++);
}

void cpu_ldhl_nn() {
   TIME(3);
   cpu.l = rbyte(cpu.pc++);
   cpu.h = rbyte(cpu.pc++);
}

void cpu_ldsp_nn() {
   TIME(3);
   cpu.sp = rword(cpu.pc);
   cpu.pc += 2;
}

void cpu_ldsp_hl() {
   TIME(2);
   cpu.sp = (cpu.h << 8) | cpu.l;
}

void cpu_ldhl_sp_n() {
   TIME(2);
   byte next = rbyte(cpu.pc++);
   TIME(1);
   sbyte off = (sbyte)next;
   int res   = off + cpu.sp;
   CLEAR_FLAGS();
   cpu.cf = ((cpu.sp & 0xFF) + next > 0xFF);
   cpu.hf = ((cpu.sp & 0xF) + (next & 0xF) > 0xF);
   cpu.h  = (res & 0xFF00) >> 8;
   cpu.l  = res & 0x00FF;
}

void cpu_ld_nn_sp() {
   TIME(5);
   word tmp = rword(cpu.pc);
   cpu.pc += 2;
   wword(tmp, cpu.sp);
}

// PUSH / POP

void cpu_pushbc() {
   TIME(2);
   PUSH(cpu.b, cpu.c);
}
void cpu_pushde() {
   TIME(2);
   PUSH(cpu.d, cpu.e);
}
void cpu_pushhl() {
   TIME(2);
   PUSH(cpu.h, cpu.l);
}
void cpu_popbc() {
   TIME(1);
   POP(cpu.b, cpu.c);
}
void cpu_popde() {
   TIME(1);
   POP(cpu.d, cpu.e);
}
void cpu_pophl() {
   TIME(1);
   POP(cpu.h, cpu.l);
}

void cpu_pushaf() {
   byte flags = 0;
   if (cpu.zf) {
      flags |= BITMASK_Z;
   }
   if (cpu.cf) {
      flags |= BITMASK_C;
   }
   if (cpu.hf) {
      flags |= BITMASK_H;
   }
   if (cpu.nf) {
      flags |= BITMASK_N;
   }
   TIME(2);
   PUSH(cpu.a, flags);
}

void cpu_popaf() {
   byte flags = 0;
   TIME(1);
   POP(cpu.a, flags);
   cpu.zf = (flags & BITMASK_Z);
   cpu.cf = (flags & BITMASK_C);
   cpu.hf = (flags & BITMASK_H);
   cpu.nf = (flags & BITMASK_N);
}

// AND / OR / XOR
void cpu_and_a() {
   TIME(1);
   AND(cpu.a);
}
void cpu_and_b() {
   TIME(1);
   AND(cpu.b);
}
void cpu_and_c() {
   TIME(1);
   AND(cpu.c);
}
void cpu_and_d() {
   TIME(1);
   AND(cpu.d);
}
void cpu_and_e() {
   TIME(1);
   AND(cpu.e);
}
void cpu_and_h() {
   TIME(1);
   AND(cpu.h);
}
void cpu_and_l() {
   TIME(1);
   AND(cpu.l);
}
void cpu_or_a() {
   TIME(1);
   OR(cpu.a);
}
void cpu_or_b() {
   TIME(1);
   OR(cpu.b);
}
void cpu_or_c() {
   TIME(1);
   OR(cpu.c);
}
void cpu_or_d() {
   TIME(1);
   OR(cpu.d);
}
void cpu_or_e() {
   TIME(1);
   OR(cpu.e);
}
void cpu_or_h() {
   TIME(1);
   OR(cpu.h);
}
void cpu_or_l() {
   TIME(1);
   OR(cpu.l);
}
void cpu_xor_a() {
   TIME(1);
   XOR(cpu.a);
}
void cpu_xor_b() {
   TIME(1);
   XOR(cpu.b);
}
void cpu_xor_c() {
   TIME(1);
   XOR(cpu.c);
}
void cpu_xor_d() {
   TIME(1);
   XOR(cpu.d);
}
void cpu_xor_e() {
   TIME(1);
   XOR(cpu.e);
}
void cpu_xor_h() {
   TIME(1);
   XOR(cpu.h);
}
void cpu_xor_l() {
   TIME(1);
   XOR(cpu.l);
}
void cpu_and_at_hl() {
   TIME(2);
   AND(FETCH(cpu.h, cpu.l));
}
void cpu_or_at_hl() {
   TIME(2);
   OR(FETCH(cpu.h, cpu.l));
}
void cpu_xor_at_hl() {
   TIME(2);
   XOR(FETCH(cpu.h, cpu.l));
}
void cpu_and_n() {
   TIME(2);
   AND(rbyte(cpu.pc++));
}
void cpu_or_n() {
   TIME(2);
   OR(rbyte(cpu.pc++));
}
void cpu_xor_n() {
   TIME(2);
   XOR(rbyte(cpu.pc++));
}

void cpu_cb() {
   TIME(2);
   byte sub_op  = rbyte(cpu.pc++);
   byte* regs[] = {
         &cpu.b, &cpu.c, &cpu.d, &cpu.e, &cpu.h, &cpu.l, NULL, &cpu.a};
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
   if (!cpu.zf) {
      jp();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_jp_z_nn() {
   if (cpu.zf) {
      jp();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_jp_nc_nn() {
   if (!cpu.cf) {
      jp();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_jp_c_nn() {
   if (cpu.cf) {
      jp();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_jp_at_hl() {
   TIME(1);
   cpu.pc = ((cpu.h << 8) | cpu.l);
}

void cpu_jr_n() {
   TIME(3);
   JR();
}

void cpu_jr_nz_n() {
   if (!cpu.zf) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu.pc++;
   }
}

void cpu_jr_z_n() {
   if (cpu.zf) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu.pc++;
   }
}

void cpu_jr_nc_n() {
   if (!cpu.cf) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu.pc++;
   }
}

void cpu_jr_c_n() {
   if (cpu.cf) {
      TIME(3);
      JR();
   } else {
      TIME(2);
      cpu.pc++;
   }
}

void cpu_call_nn() { call(); }

void cpu_call_nz_nn() {
   if (!cpu.zf) {
      call();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_call_z_nn() {
   if (cpu.zf) {
      call();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_call_nc_nn() {
   if (!cpu.cf) {
      call();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

void cpu_call_c_nn() {
   if (cpu.cf) {
      call();
   } else {
      TIME(3);
      cpu.pc += 2;
   }
}

// Restarts

void cpu_rst_00h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x00;
}

void cpu_rst_08h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x08;
}

void cpu_rst_10h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x10;
}

void cpu_rst_18h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x18;
}

void cpu_rst_20h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x20;
}

void cpu_rst_28h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x28;
}

void cpu_rst_30h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x30;
}

void cpu_rst_38h() {
   TIME(2);
   PUSHW(cpu.pc);
   cpu.halted  = false;
   cpu.stopped = false;
   cpu.pc      = 0x38;
}

// Returns

void cpu_ret() { ret(); }

void cpu_ret_nz() {
   if (!cpu.zf) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_ret_z() {
   if (cpu.zf) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_ret_nc() {
   if (!cpu.cf) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_ret_c() {
   if (cpu.cf) {
      TIME(1);
      ret();
   } else {
      TIME(2);
   }
}

void cpu_reti() {
   cpu.ime       = true;
   cpu.ime_delay = false;
   ret();
}

// 16 bit math

void cpu_add16_hl_bc() {
   TIME(2);
   add16(&cpu.h, &cpu.l, cpu.b, cpu.c);
}

void cpu_add16_hl_de() {
   TIME(2);
   add16(&cpu.h, &cpu.l, cpu.d, cpu.e);
}

void cpu_add16_hl_hl() {
   TIME(2);
   add16(&cpu.h, &cpu.l, cpu.h, cpu.l);
}

void cpu_add16_hl_sp() {
   TIME(2);
   add16(&cpu.h, &cpu.l, (cpu.sp & 0xFF00) >> 8, cpu.sp & 0x00FF);
}

void cpu_add16_sp_n() {
   TIME(2);
   byte val = rbyte(cpu.pc++);
   TIME(2);
   sbyte off = (sbyte)val;
   CLEAR_FLAGS();
   cpu.hf = ((cpu.sp & 0xF) + (val & 0xF) > 0xF);
   cpu.cf = (((cpu.sp & 0xFF) + val > 0xFF));
   cpu.sp += off;
}

// 0x03
void cpu_inc16_bc() {
   TIME(2);
   inc16(&cpu.b, &cpu.c);
}

void cpu_inc16_de() {
   TIME(2);
   inc16(&cpu.d, &cpu.e);
}

void cpu_inc16_hl() {
   TIME(2);
   inc16(&cpu.h, &cpu.l);
}

void cpu_inc16_sp() {
   TIME(2);
   cpu.sp++;
}

void cpu_dec16_bc() {
   TIME(2);
   dec16(&cpu.b, &cpu.c);
}

void cpu_dec16_de() {
   TIME(2);
   dec16(&cpu.d, &cpu.e);
}

void cpu_dec16_hl() {
   TIME(2);
   dec16(&cpu.h, &cpu.l);
}

void cpu_dec16_sp() {
   TIME(2);
   cpu.sp--;
}

// Rotates / misc

void cpu_rla() {
   TIME(1);
   rl(&cpu.a);
   cpu.zf = false;
}

void cpu_rlca() {
   TIME(1);
   rlc(&cpu.a);
   cpu.zf = false;
}

void cpu_rrca() {
   TIME(1);
   rrc(&cpu.a);
   cpu.zf = false;
}

void cpu_rra() {
   TIME(1);
   rr(&cpu.a);
   cpu.zf = false;
}

void cpu_di() {
   //   //printf("[%04X] DI, %02X, %d, %lld\n", cpu.pc - 1,
   //   rbyte(LCD_STATUS_ADDR), ppu_ly, ppu_get_timer());
   TIME(1);
   cpu.ime       = false;
   cpu.ime_delay = false;
}

void cpu_ei() {
   //   //printf("[%04X] EI, %02X, %d, %lld\n", cpu.pc - 1,
   //   rbyte(LCD_STATUS_ADDR), ppu_ly, ppu_get_timer());
   TIME(1);
   cpu.ime       = true;
   cpu.ime_delay = true;
}

// ADD / ADC / SUB / SUBC
void cpu_add_a_a() {
   TIME(1);
   ADD(cpu.a);
}
void cpu_add_a_b() {
   TIME(1);
   ADD(cpu.b);
}
void cpu_add_a_c() {
   TIME(1);
   ADD(cpu.c);
}
void cpu_add_a_d() {
   TIME(1);
   ADD(cpu.d);
}
void cpu_add_a_e() {
   TIME(1);
   ADD(cpu.e);
}
void cpu_add_a_h() {
   TIME(1);
   ADD(cpu.h);
}
void cpu_add_a_l() {
   TIME(1);
   ADD(cpu.l);
}
void cpu_adc_a_a() {
   TIME(1);
   ADC(cpu.a);
}
void cpu_adc_a_b() {
   TIME(1);
   ADC(cpu.b);
}
void cpu_adc_a_c() {
   TIME(1);
   ADC(cpu.c);
}
void cpu_adc_a_d() {
   TIME(1);
   ADC(cpu.d);
}
void cpu_adc_a_e() {
   TIME(1);
   ADC(cpu.e);
}
void cpu_adc_a_h() {
   TIME(1);
   ADC(cpu.h);
}
void cpu_adc_a_l() {
   TIME(1);
   ADC(cpu.l);
}
void cpu_sub_a_a() {
   TIME(1);
   SUB(cpu.a);
}
void cpu_sub_a_b() {
   TIME(1);
   SUB(cpu.b);
}
void cpu_sub_a_c() {
   TIME(1);
   SUB(cpu.c);
}
void cpu_sub_a_d() {
   TIME(1);
   SUB(cpu.d);
}
void cpu_sub_a_e() {
   TIME(1);
   SUB(cpu.e);
}
void cpu_sub_a_h() {
   TIME(1);
   SUB(cpu.h);
}
void cpu_sub_a_l() {
   TIME(1);
   SUB(cpu.l);
}
void cpu_sbc_a_a() {
   TIME(1);
   SBC(cpu.a);
}
void cpu_sbc_a_b() {
   TIME(1);
   SBC(cpu.b);
}
void cpu_sbc_a_c() {
   TIME(1);
   SBC(cpu.c);
}
void cpu_sbc_a_d() {
   TIME(1);
   SBC(cpu.d);
}
void cpu_sbc_a_e() {
   TIME(1);
   SBC(cpu.e);
}
void cpu_sbc_a_h() {
   TIME(1);
   SBC(cpu.h);
}
void cpu_sbc_a_l() {
   TIME(1);
   SBC(cpu.l);
}
void cpu_add_a_n() {
   TIME(2);
   ADD(rbyte(cpu.pc++));
}
void cpu_adc_a_n() {
   TIME(2);
   ADC(rbyte(cpu.pc++));
}
void cpu_sub_a_n() {
   TIME(2);
   SUB(rbyte(cpu.pc++));
}
void cpu_sbc_a_n() {
   TIME(2);
   SBC(rbyte(cpu.pc++));
}
void cpu_add_a_at_hl() {
   TIME(2);
   ADD(FETCH(cpu.h, cpu.l));
}
void cpu_adc_a_at_hl() {
   TIME(2);
   ADC(FETCH(cpu.h, cpu.l));
}
void cpu_sub_a_at_hl() {
   TIME(2);
   SUB(FETCH(cpu.h, cpu.l));
}
void cpu_sbc_a_at_hl() {
   TIME(2);
   SBC(FETCH(cpu.h, cpu.l));
}

// INCREMENT / DECREMENT
void cpu_inc_a() {
   INC(cpu.a);
   TIME(1);
}
void cpu_inc_b() {
   INC(cpu.b);
   TIME(1);
}
void cpu_inc_c() {
   INC(cpu.c);
   TIME(1);
}
void cpu_inc_d() {
   INC(cpu.d);
   TIME(1);
}
void cpu_inc_e() {
   INC(cpu.e);
   TIME(1);
}
void cpu_inc_h() {
   TIME(1);
   INC(cpu.h);
}
void cpu_inc_l() {
   INC(cpu.l);
   TIME(1);
}
void cpu_dec_a() {
   DEC(cpu.a);
   TIME(1);
}
void cpu_dec_b() {
   DEC(cpu.b);
   TIME(1);
}
void cpu_dec_c() {
   DEC(cpu.c);
   TIME(1);
}
void cpu_dec_d() {
   DEC(cpu.d);
   TIME(1);
}
void cpu_dec_e() {
   DEC(cpu.e);
   TIME(1);
}
void cpu_dec_h() {
   DEC(cpu.h);
   TIME(1);
}
void cpu_dec_l() {
   DEC(cpu.l);
   TIME(1);
}

void cpu_inc_at_hl() {
   TIME(2);
   byte val = FETCH(cpu.h, cpu.l);
   cpu.hf   = (val & 0x0F) + 1 > 0x0F;
   cpu.nf   = false;
   val++;
   cpu.zf = val == 0;
   TIME(1);
   STORE(cpu.h, cpu.l, val);
}

void cpu_dec_at_hl() {
   TIME(2);
   byte val = FETCH(cpu.h, cpu.l);
   cpu.hf   = (val & 0x0F) == 0;
   val--;
   cpu.zf = val == 0;
   cpu.nf = true;
   TIME(1);
   STORE(cpu.h, cpu.l, val);
}

// COMPARE
void cpu_cp_a() {
   TIME(1);
   COMPARE(cpu.a);
}
void cpu_cp_b() {
   TIME(1);
   COMPARE(cpu.b);
}
void cpu_cp_c() {
   TIME(1);
   COMPARE(cpu.c);
}
void cpu_cp_d() {
   TIME(1);
   COMPARE(cpu.d);
}
void cpu_cp_e() {
   TIME(1);
   COMPARE(cpu.e);
}
void cpu_cp_h() {
   TIME(1);
   COMPARE(cpu.h);
}
void cpu_cp_l() {
   TIME(1);
   COMPARE(cpu.l);
}
void cpu_cp_at_hl() {
   TIME(2);
   COMPARE(FETCH(cpu.h, cpu.l));
}
void cpu_cp_a_n() {
   TIME(2);
   COMPARE(rbyte(cpu.pc++));
}

void cpu_cpl() {
   TIME(1);
   cpu.a  = ~cpu.a;
   cpu.hf = true;
   cpu.nf = true;
}

void cpu_ccf() {
   TIME(1);
   cpu.cf = !cpu.cf;
   cpu.nf = false;
   cpu.hf = false;
}

void cpu_scf() {
   TIME(1);
   cpu.hf = false;
   cpu.nf = false;
   cpu.cf = true;
}

void cpu_halt() {
   TIME(1);
   cpu.halted = true;
}

void cpu_stop() {
   TIME(1);
   cpu.stopped = true;
}

// DAA implementation based on code found at
// http://forums.nesdev.com/viewtopic.php?t=9088
void cpu_daa() {
   TIME(1);
   int a = cpu.a;
   if (!cpu.nf) {
      if (cpu.hf || (a & 0xF) > 9) {
         a += 0x06;
      }
      if (cpu.cf || a > 0x9F) {
         a += 0x60;
      }
   } else {
      if (cpu.hf) {
         a = (a - 6) & 0xFF;
      }
      if (cpu.cf) {
         a -= 0x60;
      }
   }
   cpu.a  = (byte)a;
   cpu.hf = false;
   cpu.zf = !cpu.a;
   if ((a & 0x100) == 0x100) {
      cpu.cf = true;
   }
}

#endif
