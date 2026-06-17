/*
 * File: texture.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Textures!
 */
#include "sol_core.h"

#include "texture.h"
#include "render/render_i.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "webp/decode.h"

#define MAX_GLOBAL_TEXTURES 1024

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
};

SolTexture      loaded_images[MAX_GLOBAL_TEXTURES];
static uint32_t next_free_texture_idx = SOL_TEXTURE_COUNT;

static SolTexture *Parse_Texture(void *data, size_t size, const char *extension, u32 id)
{
    SolTexture *image = &loaded_images[id];
    image->data       = data;

    if (extension && strstr(extension, ".raw"))
    {
        image->pixels = data;
        image->height = 224;
        image->width  = 224;
        image->loaded = true;
        printf("ID: %d, Image upload width:%d\n", id, image->width);
        return image;
    }

    if (extension && strstr(extension, ".webp"))
    {
        image->pixels = WebPDecodeRGBA(data, size, &image->width, &image->height);
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
        Sol_Render_UploadImage(image->width, image->height, image->pixels, i);
    }
    return 0;
}

uint32_t Sol_Texture_RegisterRuntime(void *data, size_t size, const char *hint_extension)
{
    if (next_free_texture_idx >= MAX_GLOBAL_TEXTURES)
    {
        printf("Error: Global texture registry full!\n");
        return 0; // Return a default fallback texture index
    }

    // Deduplicate by checking if an image with the exact same size and starting bytes already exists
    for (uint32_t i = 0; i < next_free_texture_idx; i++)
    {
        // 1. Static images might not have a tracked payload size, so check if data pointer is identical first
        if (loaded_images[i].data == data)
            return i;

        // 2. For runtime glb chunks, match size and the first 16 signature bytes
        // (Saves you from running an expensive full memcmp over megabytes of image bytes)
        if (loaded_images[i].loaded && loaded_images[i].width > 0)
        {
            // You can add a .size field to your SolTexture struct to make this 100% robust:
            if (loaded_images[i].size == size && memcmp(loaded_images[i].pixels, ((SolTexture *)data)->pixels, 16) == 0)
                return i;
        }
    }

    uint32_t assignedSlot = next_free_texture_idx++;

    // Parse the embedded JPEG/PNG/WebP straight into our pool
    SolTexture *image = Parse_Texture(data, size, hint_extension, assignedSlot);

    Sol_UploadImage(image, assignedSlot);

    // --- VULKAN WORK ---
    // 1. Create your VkImage / VkImageView for loaded_images[assignedSlot]
    // 2. Transition layout, stage pixels to GPU memory
    // 3. Update your Global Bindless Descriptor Set at index `assignedSlot`

    return assignedSlot; // This ID goes into your MeshMaterial push constant!
}