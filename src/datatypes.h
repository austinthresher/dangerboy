#ifndef _DATATYPES_H
#define _DATATYPES_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define byte uint8_t
#define word uint16_t
#define tick uint64_t
#define sbyte int8_t

#define ERROR(...)                                 \
   printf("Error at %s:%d: ", __FILE__, __LINE__); \
   printf(__VA_ARGS__);                            \
   exit(1)

#endif
