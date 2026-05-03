#pragma once
#include "sol/types.h"
#include "font/font.h"
#include "model/model.h"

typedef struct SolBank
{
    SolFont  fonts[SOL_FONT_COUNT];
    SolImage images[SOL_IMAGE_COUNT];
    SolModel models[SOL_MODEL_COUNT];
} SolBank;

extern SolBank bank;

SolBank *Sol_Getbank();
SOLAPI SolModel *Sol_GetModel(SolModelId id);
