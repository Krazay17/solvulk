// sol_array.h
#include <stdlib.h>
#include <string.h>

#define SolArray(T)                                                                                                    \
    struct                                                                                                             \
    {                                                                                                                  \
        T  *data;                                                                                                      \
        u32 count;                                                                                                     \
        u32 capacity;                                                                                                  \
    }

#define SolArray_Init(arr, initial_cap)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        (arr)->count    = 0;                                                                                           \
        (arr)->capacity = (initial_cap);                                                                               \
        (arr)->data     = malloc(sizeof(*(arr)->data) * (initial_cap));                                                \
    } while (0)

#define SolArray_Push(arr, value)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((arr)->count >= (arr)->capacity)                                                                           \
        {                                                                                                              \
            (arr)->capacity = (arr)->capacity ? (arr)->capacity * 2 : 16;                                              \
            (arr)->data     = realloc((arr)->data, sizeof(*(arr)->data) * (arr)->capacity);                            \
        }                                                                                                              \
        (arr)->data[(arr)->count++] = (value);                                                                         \
    } while (0)

#define SolArray_Free(arr)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        free((arr)->data);                                                                                             \
        (arr)->data     = NULL;                                                                                        \
        (arr)->count    = 0;                                                                                           \
        (arr)->capacity = 0;                                                                                           \
    } while (0)