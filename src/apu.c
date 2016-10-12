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

byte apu_reg_read(word addr) {
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
      case CH1VOLUME: // TODO: Envelope is a more accurate name
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
      case CH2VOLUME: 
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
         ch4_len = val;
         return;
      case CH4VOLUME: 
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
