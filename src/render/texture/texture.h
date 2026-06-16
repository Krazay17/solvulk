#pragma once
#include "sol/types.h"

typedef struct SolTexture
{
    SolTextureId id;
    void        *pixels, *data;
    u32          width, height;
    u32          channels;
    bool         loaded;
} SolTexture;

int Sol_Textures_Init();

SolTexture *Sol_GetImage(u32 id);
uint32_t    Sol_Texture_RegisterRuntime(void *data, size_t size, const char *hint_extension);