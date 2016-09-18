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

#define ERROR(...) do { printf("Error at %s:%d: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__); \
        raise_error(); } while(0)

#endif
