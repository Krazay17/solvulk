#include "sol_core.h"

const char *image_path[SOL_IMAGE_COUNT] = {
    [SOL_IMAGE_FONT] = "atlas.raw",
};

SolImage loaded_images[SOL_IMAGE_COUNT];

SolImage *Sol_GetImage(SolImageId id)
{
    return &loaded_images[id];
}

void Sol_Load_Textures()
{
    for (int i = 0; i < SOL_IMAGE_COUNT; i++)
    {
        SolResource res   = Sol_LoadResource(image_path[i]);
        SolImage   *image = Parse_Texture(res, i);
        Sol_UploadImage(image, i);
    }
}

SolImage *Parse_Texture(SolResource res, u32 id)
{
    SolImage *image = &loaded_images[id];
    image->pixels   = res.data;
    image->width    = 224;
    image->height   = 224;

    return image;
}