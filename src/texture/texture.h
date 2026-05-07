#pragma once

typedef enum
{
    SOL_TEXTURE_ICEFONT,
    SOL_TEXTURE_COUNT,
} SolTextureId;

typedef struct
{
    SolTextureId id;
    void        *pixels;
    u32          width, height;
} SolTexture;

const char *image_path[SOL_IMAGE_COUNT];
SolImage    loaded_images[SOL_IMAGE_COUNT];

SolImage *Sol_GetImage(SolImageId id);

void      Sol_Load_Textures();
SolImage *Parse_Texture(SolResource res, u32 id);
