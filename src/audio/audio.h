#pragma once
#include "sol/types.h"

typedef struct
{
    char  name[64];
    float duration;
} SolAudio;

SolAudio Parse_Audio(SolResource res, u32 id);