#pragma once

#include "cglm/include/cglm/types-struct.h"
#include <stdbool.h>
#include <float.h>
#include <stdint.h>

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOL_VERSION 1
#define FLOATING_EPSILON 1e-7f
#define BITC(x) (1ULL << (x))
#define VEC4_WHITE (vec4s){1.0f, 1.0f, 1.0f, 1.0f}
#define VEC4_BLACK (vec4s){0.0f, 0.0f, 0.0f, 1.0f}
#define VEC4_RED (vec4s){1.0f, 0.0f, 0.0f, 1.0f}

// static inline void _sollog_int(int v)
// {
//     printf("%d\n", v);
// }
// static inline void _sollog_bool(bool v)
// {
//     printf("%s\n", v ? "true" : "false");
// }
// static inline void _sollog_u32(uint32_t v)
// {
//     printf("%u\n", v);
// }
// static inline void _sollog_float(double v)
// {
//     printf("%f\n", v);
// }
// static inline void _sollog_size_t(size_t v)
// {
//     printf("%zu bytes\n", v);
// }
// static inline void _sollog_str(const char *v)
// {
//     printf("%s\n", v ? v : "(null)");
// }

// #define sollog(X)                                                                                                      \
//     _Generic((1 ? (X) : (X)),                                                                                          \
//         int: _sollog_int,                                                                                              \
//         bool: _sollog_bool,                                                                                            \
//         uint32_t: _sollog_u32,                                                                                         \
//         float: _sollog_float,                                                                                          \
//         double: _sollog_float,                                                                                         \
//         size_t: _sollog_size_t,                                                                                        \
//         char *: _sollog_str,                                                                                           \
//         const char *: _sollog_str)(X)

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef unsigned char byte;
typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;

typedef struct World World;
typedef void (*UpdateFunc)(World *, double, double);

static const float ONE_THIRD = 1.0f / 3.0f;

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Helper functions (use spaces instead of newlines)
static inline void _sollog_int(int v)
{
    printf("%d ", v);
}
static inline void _sollog_bool(bool v)
{
    printf("%s ", v ? "true" : "false");
}
static inline void _sollog_u32(uint32_t v)
{
    printf("%u ", v);
}
static inline void _sollog_float(double v)
{
    printf("%f ", v);
}
static inline void _sollog_size_t(size_t v)
{
    printf("%zu ", v);
}
static inline void _sollog_str(const char *v)
{
    printf("%s ", v ? v : "(null)");
}
static inline void _sollog_vec3(vec3s v)
{
    printf("(%.2f, %.2f, %.2f) ", v.x, v.y, v.z);
}

// Type dispatch selector
#define _SOLLOG_DISPATCH(X)                                                                                            \
    _Generic((1 ? (X) : (X)),                                                                                          \
        int: _sollog_int,                                                                                              \
        bool: _sollog_bool,                                                                                            \
        uint32_t: _sollog_u32,                                                                                         \
        float: _sollog_float,                                                                                          \
        double: _sollog_float,                                                                                         \
        size_t: _sollog_size_t,                                                                                        \
        char *: _sollog_str,                                                                                           \
        vec3s: _sollog_vec3,                                                                                           \
        const char *: _sollog_str)(X)

// Macro expansion loop (Supports up to 8 arguments)
#define _SOLLOG_1(x) _SOLLOG_DISPATCH(x);
#define _SOLLOG_2(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_1(__VA_ARGS__)
#define _SOLLOG_3(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_2(__VA_ARGS__)
#define _SOLLOG_4(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_3(__VA_ARGS__)
#define _SOLLOG_5(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_4(__VA_ARGS__)
#define _SOLLOG_6(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_5(__VA_ARGS__)
#define _SOLLOG_7(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_6(__VA_ARGS__)
#define _SOLLOG_8(x, ...)                                                                                              \
    _SOLLOG_DISPATCH(x);                                                                                               \
    _SOLLOG_7(__VA_ARGS__)

// Argument counter
#define _SOLLOG_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define _SOLLOG_NARG(...) _SOLLOG_ARG_N(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

// Macro dispatch glue
#define _SOLLOG_CONCAT(a, b) a##b
#define _SOLLOG_SELECT(N) _SOLLOG_CONCAT(_SOLLOG_, N)

// Final public API
#define sollog(...)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        _SOLLOG_SELECT(_SOLLOG_NARG(__VA_ARGS__))(__VA_ARGS__);                                                        \
        printf("\n");                                                                                                  \
    } while (0)
