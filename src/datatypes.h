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

#define ERROR(...) do { printf("Error at %s:%d: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__); \
        raise_error(); } while(0)

#ifndef DEBUG_OUTPUT
#  define DEBUG(...) {}
#else
#  define DEBUG(...) do { printf("%20" PRIu64 " :\t", get_debug_time());\
                          printf(__VA_ARGS__); } while(0)
#endif

#endif
