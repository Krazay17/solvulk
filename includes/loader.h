#pragma once
#include "model.h"
#include "render.h"

typedef struct
{
    SolModel wizard;
    SolGpuModel gpuWizard;
} Models;

typedef struct
{
    Models models;
} SolBank;

void LoadModels();

SolBank *GetBank(void);