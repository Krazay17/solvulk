#pragma once

typedef struct SolResource
{
    void *data;
    long  size;
    int   isHeap;
} SolResource;

SolResource Sol_LoadResource(const char *resourceName);
int         Sol_ReadFile(const char *filename, SolResource *outRes);

int Sol_Fonts_Init();
