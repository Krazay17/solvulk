/*
 * File: texture.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Textures!
 */
#include "image.h"
#include "sol_core.h"
#include "platform/platform.h"
#include "render/render_i.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define JEBP_IMPLEMENTATION
#include "jebp.h"
// #include "webp/decode.h"

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
    [SOL_TEXTURE_CLOUD2]           = "Cloud2.webp",
    [SOL_TEXTURE_BORDER]           = "Border.webp",
    [SOL_TEXTURE_DASH_CARD]        = "DashCard.png",
    [SOL_TEXTURE_SPIN_CARD]        = "TornadoCard.png",
    [SOL_TEXTURE_LIGHTNING]        = "Lightning.webp",
    [SOL_TEXTURE_BEAM]             = "Beam.webp",
    [SOL_TEXTURE_IMPACT]           = "Impact.webp",
    [SOL_TEXTURE_LASER_CARD]       = "LaserCard.webp",
    [SOL_TEXTURE_CROSSHAIR]        = "Crosshair.png",
    [SOL_TEXTURE_FOGSTRIP]         = "FogStrip.png",
    [SOL_TEXTURE_SHIELD]           = "Shield.png",
    [SOL_TEXTURE_GRID]             = "Grid.png",
};

SolTexture loaded_images[MAX_GLOBAL_TEXTURES];
uint32_t   next_free_texture_idx = SOL_TEXTURE_COUNT;

static SolTexture *Parse_Texture(void *data, size_t size, const char *extension, u32 id)
{
    SolTexture *image = &loaded_images[id];
    image->data       = data;
    sollog(extension);

    if (extension && strstr(extension, "raw"))
    {
        image->pixels = data;
        image->height = 224;
        image->width  = 224;
        image->loaded = true;
        image->unorm  = 1;
        printf("ID: %d, Image upload width:%d\n", id, image->width);
        return image;
    }

    if (extension && strstr(extension, "webp"))
    {
        // image->pixels = WebPDecodeRGBA(data, size, &image->width, &image->height);
        jebp_image_t jebp_image;
        int          err = jebp_decode(&jebp_image, size, data);
        if (err != 0)
        {
            fprintf(stderr, "jebp fail %d", err);
            return NULL;
        }

        image->width  = jebp_image.width;
        image->height = jebp_image.height;
        image->pixels = jebp_image.pixels;

        image->loaded = true;
        printf("ID: %d, Image upload width:%d\n", id, image->width);
        return image;
    }

    int w, h, channels;
    image->pixels = stbi_load_from_memory((const stbi_uc *)data,        // pointer cast, not compound literal
                                          (int)size, &w, &h, &channels, // output addresses
                                          4                             // force 4 channels (RGBA)
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

    printf("ID: %d, Image upload width:%d\n", id, image->width);

    return image;
}

SolTexture *Sol_GetImage(u32 id)
{
    return &loaded_images[id];
}

int Sol_Textures_Init()
{
    for (int i = 0; i < SOL_TEXTURE_COUNT; i++)
    {
        SolResource res   = Sol_LoadResource(image_path[i]);
        const char *ext   = strrchr(image_path[i], '.');
        SolTexture *image = Parse_Texture(res.data, res.size, ext, i);

        // FIX: Ensure size property is explicitly set for static assets!
        image->size           = res.size;
        image->needsGpuUpload = true;
        // Sol_Render_UploadImage(image->width, image->height, image->pixels, i);
    }
    return 0;
}

u32 Sol_Texture_RegisterUnormTexture(void *data, size_t size, const char *hint_extension)
{
    u32 id                  = Sol_Texture_RegisterRuntime(data, size, hint_extension);
    loaded_images[id].unorm = 1;
}

uint32_t Sol_Texture_RegisterRuntime(void *data, size_t size, const char *hint_extension)
{
    if (next_free_texture_idx >= MAX_GLOBAL_TEXTURES)
    {
        printf("Error: Global texture registry full!\n");
        return 0;
    }

    // Deduplicate by checking if we've already loaded this exact asset data chunk
    for (int i = 0; i < next_free_texture_idx; i++)
    {
        if (loaded_images[i].data == data)
            return i;

        // FIX: Verify size matches perfectly AND is greater than zero to prevent junk collisions
        if (loaded_images[i].loaded && loaded_images[i].size > 0 && loaded_images[i].size == size)
        {
            if (loaded_images[i].data && memcmp(loaded_images[i].data, data, 16) == 0)
                return i;
        }
    }

    uint32_t assignedSlot = next_free_texture_idx++;

    // Parse the embedded JPEG/PNG/WebP straight into our pool
    SolTexture *image = Parse_Texture(data, size, hint_extension, assignedSlot);

    // Save the size on the struct so the check works next time around
    image->size           = size;
    image->needsGpuUpload = true;

    // Sol_UploadImage(image, assignedSlot);

    return assignedSlot;
}