#ifndef SOL_BUF_H
#define SOL_BUF_H

/*
SIMD / Mat4 Data	Crash / Alignment Fault	Add uint32_t _pad[2] to SolBufHeader to make it 16 bytes.
Functions pushing data	Caller loses updated array address	Pass T** or pass the parent T* struct.
*/

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

typedef struct
{
    alignas(16) uint32_t capacity;
    uint32_t count;
} SolBufHeader;

// Internal header retrieval helper
#define solb__hdr(b) ((SolBufHeader *)(b) - 1)

// Query macros
#define solb_count(b) ((b) ? solb__hdr(b)->count : 0)
#define solb_capacity(b) ((b) ? solb__hdr(b)->capacity : 0)

// Array modification operations
#define solb_push(b, v) (solb__grow(b, 1), (b)[solb__hdr(b)->count++] = (v))
#define solb_pop(b) ((b) && solb__hdr(b)->count > 0 ? (b)[--solb__hdr(b)->count] : 0)
#define solb_reserve(b, n) (solb__grow(b, (n)))
#define solb_free(b) ((b) ? (free(solb__hdr(b)), (b) = NULL) : 0)
#define solb_zero(b) ((b) ? solb__hdr(b)->count = 0 : 0)
#define solb_next(b) (solb__grow(b, 1), &(b)[solb__hdr(b)->count++])
#define solb_set_count(b, n) ((b) ? (solb__hdr(b)->count = (n)) : 0)

// Grow macro that performs type-safe pointer re-assignment
#define solb__grow(b, n)                                                                                               \
    ((!(b) || solb_count(b) + (n) > solb_capacity(b)) ? (*((void **)&(b)) = solb__grow_impl((b), (n), sizeof(*(b))))   \
                                                      : 0)

#define solb_push_array(b, src_ptr, num_items)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((num_items) > 0)                                                                                           \
        {                                                                                                              \
            uint32_t _cur = solb_count(b);                                                                             \
            uint32_t _req = _cur + (num_items);                                                                        \
            solb_reserve((b), _req);                                                                                   \
            memcpy(&(b)[_cur], (src_ptr), sizeof(*(b)) * (num_items));                                                 \
            solb__hdr(b)->count = _req;                                                                                \
        }                                                                                                              \
    } while (0)

#define solb_init(b, initial_cap)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        (b) = NULL;                                                                                                    \
        if ((initial_cap) > 0)                                                                                         \
        {                                                                                                              \
            solb_reserve((b), (initial_cap));                                                                          \
        }                                                                                                              \
    } while (0)

// Internal grow implementation function
static inline void *solb__grow_impl(void *buf, uint32_t increment, size_t elem_size)
{
    uint32_t min_cap = (buf ? solb_count(buf) : 0) + increment;
    uint32_t new_cap = buf ? solb_capacity(buf) : 0;

    if (new_cap < min_cap)
    {
        // Double capacity or default to 16
        new_cap = new_cap ? new_cap * 2 : 16;
        if (new_cap < min_cap)
        {
            new_cap = min_cap;
        }

        size_t        total_size = sizeof(SolBufHeader) + (new_cap * elem_size);
        SolBufHeader *hdr        = NULL;

        if (buf)
        {
            hdr = (SolBufHeader *)realloc(solb__hdr(buf), total_size);
        }
        else
        {
            hdr = (SolBufHeader *)malloc(total_size);
            if (hdr)
                hdr->count = 0;
        }

        if (!hdr)
            return NULL;

        hdr->capacity = new_cap;
        return (void *)(hdr + 1);
    }

    return buf;
}

#endif // SOL_BUF_H