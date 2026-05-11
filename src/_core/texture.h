#pragma once
#include "sol/types.h"

typedef struct
{
    SolTextureId id;
    void        *pixels;
    u32          width, height;
} SolTexture;

SolImage *Sol_GetImage(SolImageId id);

int      Sol_Textures_Init();
