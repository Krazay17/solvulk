#pragma once
#include "model.h"
#include "render.h"

typedef struct
{
    SolModelHandle wizard;
} BankedModels;

typedef struct
{
    BankedModels models;
} SolBank;

void LoadModels();

SolBank *GetBank(void);