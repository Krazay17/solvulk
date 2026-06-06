/*
 * File: texture.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Textures!
 */
#include "sol_core.h"

#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "webp/decode.h"

const char *image_path[SOL_TEXTURE_COUNT] = {
    [SOL_TEXTURE_ICEFONT]          = "atlas.raw",
    [SOL_TEXTURE_FIREPARTICLE]     = "FireParticle.png",
    [SOL_TEXTURE_SHOCKPARTICLE]    = "ShockParticle.png",
    [SOL_TEXTURE_CLOUDPARTICLE]    = "CloudParticle.png",
    [SOL_TEXTURE_BLOODPARTICLE]    = "BloodParticle.png",
    [SOL_TEXTURE_REDSKY]           = "RedSky.webp",
    [SOL_TEXTURE_FIREBALL_CARD]    = "Fireball.png",
    [SOL_TEXTURE_PISTOL_CARD]      = "Pistol.png",
    [SOL_TEXTURE_BLADE_CARD]       = "Blade.png",
    [SOL_TEXTURE_CRYSTAL_CARD]     = "CardCrystal.png",
    [SOL_TEXTURE_CLOUD1]           = "Cloud1.webp",
    [SOL_TEXTURE_HEALTH]           = "HealthTexture.webp",
    [SOL_TEXTURE_SPIKEFRAMEFILLED] = "SpikeFrameFilled.webp",
    [SOL_TEXTURE_SWIRLFRAME]       = "SwirlFrame.webp",
};

SolTexture loaded_images[SOL_TEXTURE_COUNT];

static SolTexture *Parse_Texture(SolResource res, u32 id);

SolTexture *Sol_GetImage(SolTextureId id)
{
    return &loaded_images[id];
}

int Sol_Textures_Init()
{
    for (int i = 0; i < SOL_TEXTURE_COUNT; i++)
    {
        SolResource res   = Sol_LoadResource(image_path[i]);
        SolTexture *image = Parse_Texture(res, i);
    }
    return 0;
}

static SolTexture *Parse_Texture(SolResource res, u32 id)
{
    SolTexture *image = &loaded_images[id];

    if (strstr(image_path[id], ".raw"))
    {
        image->pixels = res.data;
        image->height = 224;
        image->width  = 224;
        image->loaded = true;
        return image;
    }

    if (strstr(image_path[id], ".webp"))
    {
        int width, height;
        int info      = WebPGetInfo(res.data, res.size, &width, &height);
        image->pixels = WebPDecodeRGBA(res.data, res.size, &image->width, &image->height);
        return image;
    }

    int w, h, channels;
    image->pixels = stbi_load_from_memory((const stbi_uc *)res.data,        // pointer cast, not compound literal
                                          (int)res.size, &w, &h, &channels, // output addresses
                                          4                                 // force 4 channels (RGBA)
    );

    if (!image->pixels)
    {
        printf("stbi failed for %d: %s\n", id, stbi_failure_reason());
        return NULL;
    }

    image->width    = (u32)w;
    image->height   = (u32)h;
    image->channels = 4; // we forced 4
    image->loaded   = true;

    printf("Image upload width:%d\n", image->width);

    return image;
}