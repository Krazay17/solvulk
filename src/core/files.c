#include "files.h"

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

SolResource SolLoadResource(const char *resourceName)
{
    SolResource res = {0};

    HRSRC hResource = FindResource(NULL, resourceName, RT_RCDATA);
    if (!hResource) {
        printf("Failed to find resource %s\n", resourceName);
        return res;
    }

    HGLOBAL hLoaded = LoadResource(NULL, hResource);
    if (!hLoaded) {
        printf("Failed to load resource %s\n", resourceName);
        return res;
    }

    res.data = LockResource(hLoaded);
    res.size = SizeofResource(NULL, hResource);

    return res;
}