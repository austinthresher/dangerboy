#ifndef __DATATYPES_H__
#define __DATATYPES_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define byte uint8_t
#define word uint16_t
#define tick uint64_t
#define sbyte int8_t

void raise_error();
bool check_error();
void inc_debug_time();
tick get_debug_time();

// TODO: Should this print to stderr?

#define DEBUG_RANGE_BEGIN 0
#define DEBUG_RANGE_END 0
// #define DEBUG_OUTPUT

#define ERROR(...) \
      printf("[%20" PRIu64 "] %s:%d: ", \
            get_debug_time(), __FILE__, __LINE__); \
      printf(__VA_ARGS__); \
      raise_error();                                         \

#ifndef DEBUG_OUTPUT
#  define DEBUG(...) {}
#else
#  define DEBUG(...) printf("%20" PRIu64 " :\t", get_debug_time());\
                     printf(__VA_ARGS__);
#endif

#endif
