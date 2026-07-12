#pragma once
#include "sol/types.h"

#define MAX_GLOBAL_TEXTURES 512

typedef struct SolTexture
{
    SolTextureId id;
    void        *pixels, *data;
    u32          width, height;
    u32          channels;
    bool         loaded, needsGpuUpload;
    size_t       size;
    u8           unorm;
} SolTexture;

extern SolTexture loaded_images[MAX_GLOBAL_TEXTURES];
extern uint32_t   next_free_texture_idx;

int Sol_Textures_Init();

SolTexture *Sol_GetImage(u32 id);
uint32_t    Sol_Texture_RegisterRuntime(void *data, size_t size, const char *hint_extension);
u32         Sol_Texture_RegisterUnormTexture(void *data, size_t size, const char *hint_extension);