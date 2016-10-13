#include "apu.h"
#include "memory.h"

// ----------------
// Internal defines
// ----------------

#define BUFFER_SIZE 512
#define TYPE int16_t

// ------------------
// Internal variables
// ------------------

TYPE buffer_left[BUFFER_SIZE];
TYPE buffer_right[BUFFER_SIZE];

// While structs seem like a good fit,
// differences between each channel make
// it easier to handle these individually.
word ch1_freq;
word ch2_freq;
word ch3_freq;
bool ch1_on;
bool ch2_on;
bool ch3_on;
bool ch4_on;
bool ch1_init;
bool ch2_init;
bool ch3_init;
bool ch4_init;
bool ch1_continue;
bool ch2_continue;
bool ch3_continue; 
bool ch4_continue;
bool ch1_sweep_dir;
byte ch1_sweep_val;
byte ch1_sweep_time;
byte ch1_duty;
byte ch2_duty;
byte ch3_duty;
byte ch4_duty;
byte ch1_env_len;
byte ch1_env_default;
bool ch1_env_dir;
byte ch2_env_len;
byte ch2_env_default;
bool ch2_env_dir;
byte ch1_len;
byte ch2_len;
byte ch3_len;
byte ch4_len;
byte ch3_vol;
byte ch4_env_len;
byte ch4_env_default;
bool ch4_env_dir;
byte ch4_poly_ratio;
bool ch4_poly_num;
byte ch4_poly_clock;

// --------------------
// Function definitions
// --------------------

// TODO: Should write-only bits return 1 or 0?
byte apu_reg_read(word addr) {
   switch (addr) {
      case CH1SWEEP:
         return ch1_sweep_val | (ch1_sweep_dir << 3) | (ch1_sweep_time << 4) | 0x80;
      case CH1LENGTH: 
         return ch1_len | (ch1_duty << 6);
      case CH1ENV: // TODO: Envelope is a more accurate name
         return ch1_env_len | (ch1_env_dir << 3) | (ch1_env_default << 4);
      case CH1LOFREQ: 
         return 0xFF; // Write only
      case CH1HIFREQ: 
         return (ch1_continue << 6) | 0xBF;
      case CH2LENGTH: 
         return (ch2_duty << 6) | 0x3F;
      case CH2ENV: 
         return ch2_env_len | (ch2_env_dir << 3) | (ch2_env_default << 4);
      case CH2LOFREQ: 
         return 0xFF;
      case CH2HIFREQ: 
         return (ch2_continue << 6) | 0xBF;
      case CH3ENABLE: 
         return (ch3_on << 7) | 0x7F;
      case CH3LENGTH: 
         return ch3_len;
      case CH3VOLUME: 
         return (ch3_vol << 5) | 0x9F;
      case CH3LODATA: 
         return 0xFF;
      case CH3HIDATA:
         return (ch3_continue << 6) | 0xBF;
      case CH4LENGTH: 
         return ch4_len | 0xC0;
      case CH4ENV: 
         return ch4_env_len | (ch4_env_dir << 3) | (ch4_env_default << 4);
      case CH4POLY:
         return ch4_poly_ratio | (ch4_poly_num << 3) | (ch4_poly_clock << 4);
      case CH4CONSEC: 
         return (ch4_continue << 6) | 0xBF;
      default:
         break;
   }
   return dread(addr);
}

void apu_reg_write(word addr, byte val) {
   switch (addr) {
      case CH1SWEEP:
         ch1_sweep_val  = val & 0x03;
         ch1_sweep_dir  = val & 0x08;
         ch1_sweep_time = (val & 0x70) >> 4;
         return;
      case CH1LENGTH: 
         ch1_len  = val & 0x3F;
         ch1_duty = (val & 0xC0) >> 6;
         return;
      case CH1ENV: // TODO: Envelope is a more accurate name
         ch1_env_len     = val & 0x03;
         ch1_env_dir     = val & 0x04;
         ch1_env_default = (val & 0xF0) >> 4;
      case CH1LOFREQ: 
         ch1_freq &= 0xFF00;
         ch1_freq |= val;
         return;
      case CH1HIFREQ: 
         ch1_init     = val & 0x80;
         ch1_continue = val & 0x40;
         ch1_freq &= 0x00FF;
         ch1_freq |= (val & 0x03);
         return;
      case CH2LENGTH: 
         ch2_len  = val & 0x3F;
         ch2_duty = (val & 0xC0) >> 6;
         return;
      case CH2ENV: 
         ch2_env_len     = val & 0x03;
         ch2_env_dir     = val & 0x04;
         ch2_env_default = (val & 0xF0) >> 4;
      case CH2LOFREQ: 
         ch2_freq &= 0xFF00;
         ch2_freq |= val;
         return;
      case CH2HIFREQ: 
         ch2_init     = val & 0x80;
         ch2_continue = val & 0x40;
         ch2_freq &= 0x00FF;
         ch2_freq |= (val & 0x03);
         return;
      case CH3ENABLE: 
         ch3_on = val & 0x80;
         return;
      case CH3LENGTH: 
         ch3_len = val;
         return;
      case CH3VOLUME: 
         ch3_vol = (val >> 5) & 0x03;
         return;
      case CH3LODATA: 
         ch3_freq &= 0xFF00;
         ch3_freq |= val;
         return;
      case CH3HIDATA:
         ch3_init     = val & 0x80;
         ch3_continue = val & 0x40;
         ch3_freq &= 0x00FF;
         ch3_freq |= (val & 0x03);
         return;
      case CH4LENGTH: 
         ch4_len = val & 0x3F;
         return;
      case CH4ENV: 
         ch4_env_len     = val & 0x03;
         ch4_env_dir     = val & 0x04;
         ch4_env_default = (val & 0xF0) >> 4;
         return;
      case CH4POLY:
         ch4_poly_ratio = val & 0x03;
         ch4_poly_num   = val & 0x04;
         ch4_poly_clock = val >> 4;
         return;
      case CH4CONSEC: 
         ch4_init     = val & 0x80;
         ch4_continue = val & 0x40;
         return;
      default:
         break;
   }

   dwrite(addr, val);
}

void apu_advance_time(cycle cycles) {

}
