#pragma once
#include "sol/types.h"

typedef enum
{
    SOL_TEXTURE_ICEFONT,
    SOL_TEXTURE_GFLAME,
    SOL_TEXTURE_SPIKEPARTICLE,
    SOL_TEXTURE_CLOUDPARTICLE,
    SOL_TEXTURE_BLOODPARTICLE,
    SOL_TEXTURE_REDSKY,
    SOL_TEXTURE_COUNT,
} SolTextureId;

typedef struct SolTexture
{
    SolTextureId id;
    void        *pixels;
    u32          width, height;
    u32          channels;
    bool         loaded;
} SolTexture;

int Sol_Textures_Init();

SolTexture *Sol_GetImage(SolTextureId id);
