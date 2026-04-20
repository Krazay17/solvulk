#pragma once
#include "sol_core.h"

typedef struct
{
    const void *data;
    long size;
} SolResource;

SOLAPI SolResource Sol_LoadResource(const char *resourceName);
SOLAPI SolModel Sol_LoadModel(const char *resourceName);