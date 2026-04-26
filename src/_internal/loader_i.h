#pragma once
#include "sol_core.h"

typedef struct
{
    const void *data;
    long        size;
} SolResource;

typedef struct
{
    SolResource metrics;
    SolResource atlas;
    u32 width, height;
} SolFont;

typedef struct
{
    SolFont fonts[SOL_FONT_COUNT];
    SolImage images[SOL_IMAGE_COUNT];
} SolBank;

SolBank *Sol_Load_Resources();
SolBank *Sol_Getbank();

SOLAPI SolResource Sol_LoadResource(const char *resourceName);
SOLAPI SolModel    Sol_LoadModel(const char *resourceName);