#pragma once

#include <cglm/types-struct.h>
#include <stdbool.h>
#include <stdint.h>

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#define SOL_VERSION 1
#define FLOATING_EPSILON 1e-7f
#define BITC(x) (1u << (x))
#define VEC4_WHITE (vec4s){1.0f, 1.0f, 1.0f, 1.0f}
#define VEC4_BLACK (vec4s){0.0f, 0.0f, 0.0f, 1.0f}

#define sollog(X)                                                                                                      \
    _Generic((X),                                                                                                      \
        int: printf("%d\n", (int)X),                                                                                   \
        u32: printf("%u\n", (unsigned int)(X)),                                                                        \
        float: printf("%f\n", X),                                                                                      \
        double: printf("%f\n", X),                                                                                     \
        char *: printf("%s\n", X))

typedef unsigned char byte;
typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;
