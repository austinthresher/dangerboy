#include "cpu.h"
#include "debugger.h"
#include "memory.h"
#include "opcodes.h"
#include "ppu.h"

tick last_tima_overflow;
word internal_timer;
bool prev_timer_bit;
void build_op_table();

void cpu_init() {
   build_op_table();
   last_tima_overflow = -1;
   internal_timer     = 0;
   prev_timer_bit     = false;
   cpu_reset();
}

void cpu_reset() {

   // These startup values are based on
   // http://gbdev.gg8.se/wiki/articles/Power_Up_Sequence
   cpu_pc         = 0x0100;
   cpu_a          = 0x01;
   cpu_b          = 0x00;
   cpu_c          = 0x13;
   cpu_d          = 0x00;
   cpu_e          = 0xD8;
   cpu_sp         = 0xFFFE;
   cpu_h          = 0x01;
   cpu_l          = 0x4D;
   cpu_ime        = false;
   cpu_ime_delay  = false;
   cpu_ticks      = 0;
   cpu_halted     = false;
   cpu_stopped    = false;
   internal_timer = 0;
   prev_timer_bit = false;

   FLAG_C = true;
   FLAG_H = true;
   FLAG_Z = true;
   FLAG_N = false;

   // Setup our in-memory registers
   wbyte(0xFF02, 0x7E); // Serial Transfer Control
   wbyte(0xFF05, 0x00); // TIMA
   wbyte(0xFF06, 0x00); // TMA
   wbyte(0xFF07, 0xF8); // TAC
   wbyte(0xFF10, 0x80); // NR10
   wbyte(0xFF11, 0xBF); // NR11
   wbyte(0xFF12, 0xF3); // NR12
   wbyte(0xFF14, 0xBF); // NR14
   wbyte(0xFF16, 0x3F); // NR21
   wbyte(0xFF17, 0x00); // NR22
   wbyte(0xFF19, 0xBF); // NR24
   wbyte(0xFF1A, 0x7F); // NR30
   wbyte(0xFF1B, 0xFF); // NR31
   wbyte(0xFF1C, 0x9F); // NR32
   wbyte(0xFF1E, 0xBF); // NR33
   wbyte(0xFF20, 0xFF); // NR41
   wbyte(0xFF21, 0x00); // NR42
   wbyte(0xFF22, 0x00); // NR43
   wbyte(0xFF23, 0xBF); // NR30
   wbyte(0xFF24, 0x77); // NR50
   wbyte(0xFF25, 0xF3); // NR51
   wbyte(0xFF26, 0xF1); // NR52
   wbyte(0xFF40, 0x91); // LCDC
   wbyte(0xFF42, 0x00); // SCY
   wbyte(0xFF43, 0x00); // SCX
   wbyte(0xFF45, 0x00); // LYC
   wbyte(0xFF47, 0xFC); // BG PAL
   wbyte(0xFF48, 0xFF); // OBJ0 PAL
   wbyte(0xFF49, 0xFF); // OBJ1 PAL
   wbyte(0xFF4A, 0x00); // WINX
   wbyte(0xFF4B, 0x00); // WINY
   wbyte(0xFFFF, 0x00); // IE
}

void cpu_reset_timer() { internal_timer = 0; }

void cpu_advance_time(tick dt) {
   ppu_advance_time(dt);
   mem_advance_time(dt);

   bool timer_on  = mem_direct_read(TAC) & 4;
   byte tac_speed = mem_direct_read(TAC) & 3;
   word timer_bit = 1;
   switch (tac_speed) {
      case 0: timer_bit <<= 9; break;
      case 1: timer_bit <<= 3; break;
      case 2: timer_bit <<= 5; break;
      case 3: timer_bit <<= 7; break;
   }
   for (int i = 0; i < dt / 4; ++i) {
      internal_timer += 4;
      cpu_ticks++;
      if (last_tima_overflow != -1 && cpu_ticks >= last_tima_overflow + 1) {
         mem_direct_write(IF, mem_direct_read(IF) | INT_TIMA);
         mem_direct_write(TIMA, mem_direct_read(TMA));
         last_tima_overflow = -1;
      }

      // The internal timer is based on a falling edge detector.
      // Which bit affects TIMA depends on the speed in TAC.
      word test_val = internal_timer & timer_bit;
      if (!timer_on) {
         test_val = 0;
      }

      if (prev_timer_bit && !test_val) {
         mem_direct_write(TIMA, (mem_direct_read(TIMA) + 1) & 0xFF);
         if (mem_direct_read(TIMA) == 0) {
            // TIMA interrupt happens 4 cycles after
            // the overflow. It holds 0 until then.
            last_tima_overflow = cpu_ticks;
         }
      }
      prev_timer_bit = test_val != 0;
   }
   mem_direct_write(DIV, internal_timer >> 8);
}

void cpu_execute_step() {
   // Check interrupts
   bool raised = false;
   byte int_IE = mem_direct_read(IE);
   byte int_IF = mem_direct_read(IF);
   byte irq    = int_IE & int_IF & INT_MASK;

   if (irq && cpu_halted) {
      cpu_halted = false;
      if (cpu_ime) {
         cpu_ime_delay = false;
      }
   }

   if ((int_IF & INT_INPUT) && cpu_stopped) {
      cpu_stopped = false;
   }

   if (cpu_ime) {
      if (!cpu_ime_delay) {
         byte target = 0x00;
         if (irq & INT_VBLANK) {
            target = 0x40;
            mem_direct_write(IF, int_IF & ~INT_VBLANK);
            debugger_log("VBLANK Interrupt");
            debugger_notify_mem_write(IF, mem_direct_read(IF));
         } else if (irq & INT_STAT) {
            target = 0x48;
            mem_direct_write(IF, int_IF & ~INT_STAT);
            debugger_log("STAT Interrupt");
            debugger_notify_mem_write(IF, mem_direct_read(IF));
         } else if (irq & INT_TIMA) {
            target = 0x50;
            mem_direct_write(IF, int_IF & ~INT_TIMA);
            debugger_log("TIMA Interrupt");
            debugger_notify_mem_write(IF, mem_direct_read(IF));
         } else if (irq & INT_SERIAL) {
            target = 0x58;
            mem_direct_write(IF, int_IF & ~INT_SERIAL);
            debugger_log("Serial interrupt");
            debugger_notify_mem_write(IF, mem_direct_read(IF));
         } else if (irq & INT_INPUT) {
            target = 0x60;
            mem_direct_write(IF, int_IF & ~INT_INPUT);
            debugger_log("Input Interrupt");
            debugger_notify_mem_write(IF, mem_direct_read(IF));
         }
         if (target != 0x00) {
            raised  = true;
            cpu_ime = false;
            PUSHW(cpu_pc);
            TIME(2);
            cpu_pc = target;
            TIME(1);
         }
      } else {
         cpu_ime_delay = false;
      }
   }

   if (!raised) {
      if (!cpu_halted && !cpu_stopped) {
         cpu_last_pc = cpu_pc;
         cpu_last_op = rbyte(cpu_pc++);
         (*cpu_opcodes[cpu_last_op])();
         debugger_notify_mem_exec(cpu_pc);
      } else {
         cpu_nop();
      }
   }
}

void build_op_table() {
   for (size_t i = 0; i < 0x100; i++) {
      cpu_opcodes[i] = &cpu_none;
   }

   cpu_opcodes[0x00] = &cpu_nop;         /* 1 */
   cpu_opcodes[0x01] = &cpu_ldbc_nn;     /* 3 */
   cpu_opcodes[0x02] = &cpu_ld_at_bc_a;  /* 2 */
   cpu_opcodes[0x03] = &cpu_inc16_bc;    /* 2 */
   cpu_opcodes[0x04] = &cpu_inc_b;       /* 1 */
   cpu_opcodes[0x05] = &cpu_dec_b;       /* 1 */
   cpu_opcodes[0x06] = &cpu_ldb_n;       /* 2 */
   cpu_opcodes[0x07] = &cpu_rlca;        /* 1 */
   cpu_opcodes[0x08] = &cpu_ld_nn_sp;    /* 5 */
   cpu_opcodes[0x09] = &cpu_add16_hl_bc; /* 2 */
   cpu_opcodes[0x0A] = &cpu_lda_at_bc;   /* 2 */
   cpu_opcodes[0x0B] = &cpu_dec16_bc;    /* 2 */
   cpu_opcodes[0x0C] = &cpu_inc_c;       /* 1 */
   cpu_opcodes[0x0D] = &cpu_dec_c;       /* 1 */
   cpu_opcodes[0x0E] = &cpu_ldc_n;       /* 2 */
   cpu_opcodes[0x0F] = &cpu_rrca;        /* 1 */

   cpu_opcodes[0x10] = &cpu_stop;        /* 0 */
   cpu_opcodes[0x11] = &cpu_ldde_nn;     /* 3 */
   cpu_opcodes[0x12] = &cpu_ld_at_de_a;  /* 2 */
   cpu_opcodes[0x13] = &cpu_inc16_de;    /* 2 */
   cpu_opcodes[0x14] = &cpu_inc_d;       /* 1 */
   cpu_opcodes[0x15] = &cpu_dec_d;       /* 1 */
   cpu_opcodes[0x16] = &cpu_ldd_n;       /* 2 */
   cpu_opcodes[0x17] = &cpu_rla;         /* 1 */
   cpu_opcodes[0x18] = &cpu_jr_n;        /* 3 */
   cpu_opcodes[0x19] = &cpu_add16_hl_de; /* 2 */
   cpu_opcodes[0x1A] = &cpu_lda_at_de;   /* 2 */
   cpu_opcodes[0x1B] = &cpu_dec16_de;    /* 2 */
   cpu_opcodes[0x1C] = &cpu_inc_e;       /* 1 */
   cpu_opcodes[0x1D] = &cpu_dec_e;       /* 1 */
   cpu_opcodes[0x1E] = &cpu_lde_n;       /* 2 */
   cpu_opcodes[0x1F] = &cpu_rra;         /* 1 */

   cpu_opcodes[0x20] = &cpu_jr_nz_n;     /* 2 */
   cpu_opcodes[0x21] = &cpu_ldhl_nn;     /* 3 */
   cpu_opcodes[0x22] = &cpu_ld_at_hli_a; /* 2 */
   cpu_opcodes[0x23] = &cpu_inc16_hl;    /* 2 */
   cpu_opcodes[0x24] = &cpu_inc_h;       /* 1 */
   cpu_opcodes[0x25] = &cpu_dec_h;       /* 1 */
   cpu_opcodes[0x26] = &cpu_ldh_n;       /* 2 */
   cpu_opcodes[0x27] = &cpu_daa;         /* 1 */
   cpu_opcodes[0x28] = &cpu_jr_z_n;      /* 2 */
   cpu_opcodes[0x29] = &cpu_add16_hl_hl; /* 2 */
   cpu_opcodes[0x2A] = &cpu_lda_at_hli;  /* 2 */
   cpu_opcodes[0x2B] = &cpu_dec16_hl;    /* 2 */
   cpu_opcodes[0x2C] = &cpu_inc_l;       /* 1 */
   cpu_opcodes[0x2D] = &cpu_dec_l;       /* 1 */
   cpu_opcodes[0x2E] = &cpu_ldl_n;       /* 2 */
   cpu_opcodes[0x2F] = &cpu_cpl;         /* 1 */

   cpu_opcodes[0x30] = &cpu_jr_nc_n;     /* 2 */
   cpu_opcodes[0x31] = &cpu_ldsp_nn;     /* 3 */
   cpu_opcodes[0x32] = &cpu_ld_at_hld_a; /* 2 */
   cpu_opcodes[0x33] = &cpu_inc16_sp;    /* 2 */
   cpu_opcodes[0x34] = &cpu_inc_at_hl;   /* 3 */
   cpu_opcodes[0x35] = &cpu_dec_at_hl;   /* 3 */
   cpu_opcodes[0x36] = &cpu_ld_at_hl_n;  /* 3 */
   cpu_opcodes[0x37] = &cpu_scf;         /* 1 */
   cpu_opcodes[0x38] = &cpu_jr_c_n;      /* 2 */
   cpu_opcodes[0x39] = &cpu_add16_hl_sp; /* 2 */
   cpu_opcodes[0x3A] = &cpu_lda_at_hld;  /* 2 */
   cpu_opcodes[0x3B] = &cpu_dec16_sp;    /* 2 */
   cpu_opcodes[0x3C] = &cpu_inc_a;       /* 1 */
   cpu_opcodes[0x3D] = &cpu_dec_a;       /* 1 */
   cpu_opcodes[0x3E] = &cpu_lda_n;       /* 2 */
   cpu_opcodes[0x3F] = &cpu_ccf;         /* 1 */

   cpu_opcodes[0x40] = &cpu_ldb_b;     /* 1 */
   cpu_opcodes[0x41] = &cpu_ldb_c;     /* 1 */
   cpu_opcodes[0x42] = &cpu_ldb_d;     /* 1 */
   cpu_opcodes[0x43] = &cpu_ldb_e;     /* 1 */
   cpu_opcodes[0x44] = &cpu_ldb_h;     /* 1 */
   cpu_opcodes[0x45] = &cpu_ldb_l;     /* 1 */
   cpu_opcodes[0x46] = &cpu_ldb_at_hl; /* 2 */
   cpu_opcodes[0x47] = &cpu_ldb_a;     /* 1 */
   cpu_opcodes[0x48] = &cpu_ldc_b;     /* 1 */
   cpu_opcodes[0x49] = &cpu_ldc_c;     /* 1 */
   cpu_opcodes[0x4A] = &cpu_ldc_d;     /* 1 */
   cpu_opcodes[0x4B] = &cpu_ldc_e;     /* 1 */
   cpu_opcodes[0x4C] = &cpu_ldc_h;     /* 1 */
   cpu_opcodes[0x4D] = &cpu_ldc_l;     /* 1 */
   cpu_opcodes[0x4E] = &cpu_ldc_at_hl; /* 2 */
   cpu_opcodes[0x4F] = &cpu_ldc_a;     /* 1 */

   cpu_opcodes[0x50] = &cpu_ldd_b;     /* 1 */
   cpu_opcodes[0x51] = &cpu_ldd_c;     /* 1 */
   cpu_opcodes[0x52] = &cpu_ldd_d;     /* 1 */
   cpu_opcodes[0x53] = &cpu_ldd_e;     /* 1 */
   cpu_opcodes[0x54] = &cpu_ldd_h;     /* 1 */
   cpu_opcodes[0x55] = &cpu_ldd_l;     /* 1 */
   cpu_opcodes[0x56] = &cpu_ldd_at_hl; /* 2 */
   cpu_opcodes[0x57] = &cpu_ldd_a;     /* 1 */
   cpu_opcodes[0x58] = &cpu_lde_b;     /* 1 */
   cpu_opcodes[0x59] = &cpu_lde_c;     /* 1 */
   cpu_opcodes[0x5A] = &cpu_lde_d;     /* 1 */
   cpu_opcodes[0x5B] = &cpu_lde_e;     /* 1 */
   cpu_opcodes[0x5C] = &cpu_lde_h;     /* 1 */
   cpu_opcodes[0x5D] = &cpu_lde_l;     /* 1 */
   cpu_opcodes[0x5E] = &cpu_lde_at_hl; /* 2 */
   cpu_opcodes[0x5F] = &cpu_lde_a;     /* 1 */

   cpu_opcodes[0x60] = &cpu_ldh_b;     /* 1 */
   cpu_opcodes[0x61] = &cpu_ldh_c;     /* 1 */
   cpu_opcodes[0x62] = &cpu_ldh_d;     /* 1 */
   cpu_opcodes[0x63] = &cpu_ldh_e;     /* 1 */
   cpu_opcodes[0x64] = &cpu_ldh_h;     /* 1 */
   cpu_opcodes[0x65] = &cpu_ldh_l;     /* 1 */
   cpu_opcodes[0x66] = &cpu_ldh_at_hl; /* 2 */
   cpu_opcodes[0x67] = &cpu_ldh_a;     /* 1 */
   cpu_opcodes[0x68] = &cpu_ldl_b;     /* 1 */
   cpu_opcodes[0x69] = &cpu_ldl_c;     /* 1 */
   cpu_opcodes[0x6A] = &cpu_ldl_d;     /* 1 */
   cpu_opcodes[0x6B] = &cpu_ldl_e;     /* 1 */
   cpu_opcodes[0x6C] = &cpu_ldl_h;     /* 1 */
   cpu_opcodes[0x6D] = &cpu_ldl_l;     /* 1 */
   cpu_opcodes[0x6E] = &cpu_ldl_at_hl; /* 2 */
   cpu_opcodes[0x6F] = &cpu_ldl_a;     /* 1 */

   cpu_opcodes[0x70] = &cpu_ld_at_hl_b; /* 2 */
   cpu_opcodes[0x71] = &cpu_ld_at_hl_c; /* 2 */
   cpu_opcodes[0x72] = &cpu_ld_at_hl_d; /* 2 */
   cpu_opcodes[0x73] = &cpu_ld_at_hl_e; /* 2 */
   cpu_opcodes[0x74] = &cpu_ld_at_hl_h; /* 2 */
   cpu_opcodes[0x75] = &cpu_ld_at_hl_l; /* 2 */
   cpu_opcodes[0x76] = &cpu_halt;       /* 0 */
   cpu_opcodes[0x77] = &cpu_ld_at_hl_a; /* 2 */
   cpu_opcodes[0x78] = &cpu_lda_b;      /* 1 */
   cpu_opcodes[0x79] = &cpu_lda_c;      /* 1 */
   cpu_opcodes[0x7A] = &cpu_lda_d;      /* 1 */
   cpu_opcodes[0x7B] = &cpu_lda_e;      /* 1 */
   cpu_opcodes[0x7C] = &cpu_lda_h;      /* 1 */
   cpu_opcodes[0x7D] = &cpu_lda_l;      /* 1 */
   cpu_opcodes[0x7E] = &cpu_lda_at_hl;  /* 2 */
   cpu_opcodes[0x7F] = &cpu_lda_a;      /* 1 */

   cpu_opcodes[0x80] = &cpu_add_a_b;     /* 1 */
   cpu_opcodes[0x81] = &cpu_add_a_c;     /* 1 */
   cpu_opcodes[0x82] = &cpu_add_a_d;     /* 1 */
   cpu_opcodes[0x83] = &cpu_add_a_e;     /* 1 */
   cpu_opcodes[0x84] = &cpu_add_a_h;     /* 1 */
   cpu_opcodes[0x85] = &cpu_add_a_l;     /* 1 */
   cpu_opcodes[0x86] = &cpu_add_a_at_hl; /* 2 */
   cpu_opcodes[0x87] = &cpu_add_a_a;     /* 1 */
   cpu_opcodes[0x88] = &cpu_adc_a_b;     /* 1 */
   cpu_opcodes[0x89] = &cpu_adc_a_c;     /* 1 */
   cpu_opcodes[0x8A] = &cpu_adc_a_d;     /* 1 */
   cpu_opcodes[0x8B] = &cpu_adc_a_e;     /* 1 */
   cpu_opcodes[0x8C] = &cpu_adc_a_h;     /* 1 */
   cpu_opcodes[0x8D] = &cpu_adc_a_l;     /* 1 */
   cpu_opcodes[0x8E] = &cpu_adc_a_at_hl; /* 2 */
   cpu_opcodes[0x8F] = &cpu_adc_a_a;     /* 1 */

   cpu_opcodes[0x90] = &cpu_sub_a_b;     /* 1 */
   cpu_opcodes[0x91] = &cpu_sub_a_c;     /* 1 */
   cpu_opcodes[0x92] = &cpu_sub_a_d;     /* 1 */
   cpu_opcodes[0x93] = &cpu_sub_a_e;     /* 1 */
   cpu_opcodes[0x94] = &cpu_sub_a_h;     /* 1 */
   cpu_opcodes[0x95] = &cpu_sub_a_l;     /* 1 */
   cpu_opcodes[0x96] = &cpu_sub_a_at_hl; /* 2 */
   cpu_opcodes[0x97] = &cpu_sub_a_a;     /* 1 */
   cpu_opcodes[0x98] = &cpu_sbc_a_b;     /* 1 */
   cpu_opcodes[0x99] = &cpu_sbc_a_c;     /* 1 */
   cpu_opcodes[0x9A] = &cpu_sbc_a_d;     /* 1 */
   cpu_opcodes[0x9B] = &cpu_sbc_a_e;     /* 1 */
   cpu_opcodes[0x9C] = &cpu_sbc_a_h;     /* 1 */
   cpu_opcodes[0x9D] = &cpu_sbc_a_l;     /* 1 */
   cpu_opcodes[0x9E] = &cpu_sbc_a_at_hl; /* 2 */
   cpu_opcodes[0x9F] = &cpu_sbc_a_a;     /* 1 */

   cpu_opcodes[0xA0] = &cpu_and_b;     /* 1 */
   cpu_opcodes[0xA1] = &cpu_and_c;     /* 1 */
   cpu_opcodes[0xA2] = &cpu_and_d;     /* 1 */
   cpu_opcodes[0xA3] = &cpu_and_e;     /* 1 */
   cpu_opcodes[0xA4] = &cpu_and_h;     /* 1 */
   cpu_opcodes[0xA5] = &cpu_and_l;     /* 1 */
   cpu_opcodes[0xA6] = &cpu_and_at_hl; /* 2 */
   cpu_opcodes[0xA7] = &cpu_and_a;     /* 1 */
   cpu_opcodes[0xA8] = &cpu_xor_b;     /* 1 */
   cpu_opcodes[0xA9] = &cpu_xor_c;     /* 1 */
   cpu_opcodes[0xAA] = &cpu_xor_d;     /* 1 */
   cpu_opcodes[0xAB] = &cpu_xor_e;     /* 1 */
   cpu_opcodes[0xAC] = &cpu_xor_h;     /* 1 */
   cpu_opcodes[0xAD] = &cpu_xor_l;     /* 1 */
   cpu_opcodes[0xAE] = &cpu_xor_at_hl; /* 2 */
   cpu_opcodes[0xAF] = &cpu_xor_a;     /* 1 */

   cpu_opcodes[0xB0] = &cpu_or_b;     /* 1 */
   cpu_opcodes[0xB1] = &cpu_or_c;     /* 1 */
   cpu_opcodes[0xB2] = &cpu_or_d;     /* 1 */
   cpu_opcodes[0xB3] = &cpu_or_e;     /* 1 */
   cpu_opcodes[0xB4] = &cpu_or_h;     /* 1 */
   cpu_opcodes[0xB5] = &cpu_or_l;     /* 1 */
   cpu_opcodes[0xB6] = &cpu_or_at_hl; /* 2 */
   cpu_opcodes[0xB7] = &cpu_or_a;     /* 1 */
   cpu_opcodes[0xB8] = &cpu_cp_b;     /* 1 */
   cpu_opcodes[0xB9] = &cpu_cp_c;     /* 1 */
   cpu_opcodes[0xBA] = &cpu_cp_d;     /* 1 */
   cpu_opcodes[0xBB] = &cpu_cp_e;     /* 1 */
   cpu_opcodes[0xBC] = &cpu_cp_h;     /* 1 */
   cpu_opcodes[0xBD] = &cpu_cp_l;     /* 1 */
   cpu_opcodes[0xBE] = &cpu_cp_at_hl; /* 2 */
   cpu_opcodes[0xBF] = &cpu_cp_a;     /* 1 */

   cpu_opcodes[0xC0] = &cpu_ret_nz;     /* 2 */
   cpu_opcodes[0xC1] = &cpu_popbc;      /* 3 */
   cpu_opcodes[0xC2] = &cpu_jp_nz_nn;   /* 3 */
   cpu_opcodes[0xC3] = &cpu_jp_nn;      /* 4 */
   cpu_opcodes[0xC4] = &cpu_call_nz_nn; /* 3 */
   cpu_opcodes[0xC5] = &cpu_pushbc;     /* 4 */
   cpu_opcodes[0xC6] = &cpu_add_a_n;    /* 2 */
   cpu_opcodes[0xC7] = &cpu_rst_00h;    /* 4 */
   cpu_opcodes[0xC8] = &cpu_ret_z;      /* 2 */
   cpu_opcodes[0xC9] = &cpu_ret;        /* 4 */
   cpu_opcodes[0xCA] = &cpu_jp_z_nn;    /* 3 */
   cpu_opcodes[0xCB] = &cpu_cb;         /* 0 */
   cpu_opcodes[0xCC] = &cpu_call_z_nn;  /* 3 */
   cpu_opcodes[0xCD] = &cpu_call_nn;    /* 6 */
   cpu_opcodes[0xCE] = &cpu_adc_a_n;    /* 2 */
   cpu_opcodes[0xCF] = &cpu_rst_08h;    /* 4 */

   cpu_opcodes[0xD0] = &cpu_ret_nc;     /* 2 */
   cpu_opcodes[0xD1] = &cpu_popde;      /* 3 */
   cpu_opcodes[0xD2] = &cpu_jp_nc_nn;   /* 3 */
   cpu_opcodes[0xD3] = &cpu_none;       /* 0 */
   cpu_opcodes[0xD4] = &cpu_call_nc_nn; /* 3 */
   cpu_opcodes[0xD5] = &cpu_pushde;     /* 4 */
   cpu_opcodes[0xD6] = &cpu_sub_a_n;    /* 2 */
   cpu_opcodes[0xD7] = &cpu_rst_10h;    /* 4 */
   cpu_opcodes[0xD8] = &cpu_ret_c;      /* 2 */
   cpu_opcodes[0xD9] = &cpu_reti;       /* 4 */
   cpu_opcodes[0xDA] = &cpu_jp_c_nn;    /* 3 */
   cpu_opcodes[0xDB] = &cpu_none;       /* 0 */
   cpu_opcodes[0xDC] = &cpu_call_c_nn;  /* 3 */
   cpu_opcodes[0xDD] = &cpu_none;       /* 0 */
   cpu_opcodes[0xDE] = &cpu_sbc_a_n;    /* 2 */
   cpu_opcodes[0xDF] = &cpu_rst_18h;    /* 4 */

   cpu_opcodes[0xE0] = &cpu_ld_n_a;     /* 3 */
   cpu_opcodes[0xE1] = &cpu_pophl;      /* 3 */
   cpu_opcodes[0xE2] = &cpu_ld_at_c_a;  /* 2 */
   cpu_opcodes[0xE3] = &cpu_none;       /* 0 */
   cpu_opcodes[0xE4] = &cpu_none;       /* 0 */
   cpu_opcodes[0xE5] = &cpu_pushhl;     /* 4 */
   cpu_opcodes[0xE6] = &cpu_and_n;      /* 2 */
   cpu_opcodes[0xE7] = &cpu_rst_20h;    /* 4 */
   cpu_opcodes[0xE8] = &cpu_add16_sp_n; /* 4 */
   cpu_opcodes[0xE9] = &cpu_jp_at_hl;   /* 1 */
   cpu_opcodes[0xEA] = &cpu_ld_at_nn_a; /* 4 */
   cpu_opcodes[0xEB] = &cpu_none;       /* 0 */
   cpu_opcodes[0xEC] = &cpu_none;       /* 0 */
   cpu_opcodes[0xED] = &cpu_none;       /* 0 */
   cpu_opcodes[0xEE] = &cpu_xor_n;      /* 2 */
   cpu_opcodes[0xEF] = &cpu_rst_28h;    /* 4 */

   cpu_opcodes[0xF0] = &cpu_ld_a_n;    /* 3 */
   cpu_opcodes[0xF1] = &cpu_popaf;     /* 3 */
   cpu_opcodes[0xF2] = &cpu_lda_at_c;  /* 2 */
   cpu_opcodes[0xF3] = &cpu_di;        /* 1 */
   cpu_opcodes[0xF4] = &cpu_none;      /* 0 */
   cpu_opcodes[0xF5] = &cpu_pushaf;    /* 4 */
   cpu_opcodes[0xF6] = &cpu_or_n;      /* 2 */
   cpu_opcodes[0xF7] = &cpu_rst_30h;   /* 4 */
   cpu_opcodes[0xF8] = &cpu_ldhl_sp_n; /* 3 */
   cpu_opcodes[0xF9] = &cpu_ldsp_hl;   /* 2 */
   cpu_opcodes[0xFA] = &cpu_lda_at_nn; /* 4 */
   cpu_opcodes[0xFB] = &cpu_ei;        /* 1 */
   cpu_opcodes[0xFC] = &cpu_none;      /* 0 */
   cpu_opcodes[0xFD] = &cpu_none;      /* 0 */
   cpu_opcodes[0xFE] = &cpu_cp_a_n;    /* 2 */
   cpu_opcodes[0xFF] = &cpu_rst_38h;   /* 4 */
}
