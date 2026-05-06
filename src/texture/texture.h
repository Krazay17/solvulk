#pragma once

typedef enum
{
    SOL_TEXTURE_ICEFONT,
    SOL_TEXTURE_COUNT,
} SolTextureId;

typedef struct
{
    SolTextureId id;
    void *pixels;
    float width, height;
} SolTexture;

void Parse_Texture(SolResource res, int id);