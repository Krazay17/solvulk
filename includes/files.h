#pragma once
#include <stddef.h>

typedef struct {
    const void *data;
    size_t size;
} SolResource;

//Needs free
char* SolReadFile(const char* filename, size_t* outSize);

// No free needed - memory is owned by the exe
SolResource SolLoadResource(const char *resourceName);