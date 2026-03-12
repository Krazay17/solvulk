#include "includes/files.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

char* SolReadFile(const char* filename, size_t* outSize)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        MessageBox(NULL, filename, "Failed to open shader file", MB_OK);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char* buffer = malloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    *outSize = size;
    return buffer;
}