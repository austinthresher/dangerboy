#include "datatypes.h"
#include "cpu.h"

tick debug_time   = 0;
bool system_error = false;

void raise_error() { system_error = true; }

bool check_error() { return system_error; }

void inc_debug_time() { debug_time++; }

tick get_debug_time() { return cpu_ticks; }
