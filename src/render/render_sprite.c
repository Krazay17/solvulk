#include "sol_core.h"

#include "render_i.h"

#include "render/vk/vkrender.h"

typedef struct
{
    u32      count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} SpriteQueue;

static SpriteQueue spriteQueue;
static SpriteQueue spriteFxQueue;

void Sol_Render_PushSprite(SpriteDesc desc)
{
    u32       idx;
    QuadSSBO *ssbo;

    if (desc.isfx)
    {
        idx  = spriteFxQueue.count++;
        ssbo = &spriteFxQueue.instances[idx];
    }
    else
    {
        idx  = spriteQueue.count++;
        ssbo = &spriteQueue.instances[idx];
    }

    memcpy(ssbo->pos, desc.pos.raw, sizeof(vec4));

    if (desc.color.a == 0)
        desc.color = (vec4s){{1.0f, 1.0f, 1.0f, 1.0f}};
    memcpy(ssbo->color, desc.color.raw, sizeof(vec4));
    if (desc.rotation.w == 0)
        desc.rotation = (versors){{0, 0, 0, 1.0f}};
    memcpy(ssbo->rotation, desc.rotation.raw, sizeof(versor));
    desc.uv = (vec4s){{0, 0, 1.0f, 1.0f}};
    memcpy(ssbo->uv, desc.uv.raw, sizeof(vec4));
    ssbo->type      = desc.kind;
    ssbo->textureId = desc.textureId;
}

void Flush_Sprites(void)
{
    QuadSSBO *gpu = Sol_GetDescriptorMapping(DESC_QUAD);
    
    // Copy solid sprites starting at slot 0
    u32 solidCount = spriteQueue.count;
    memcpy(gpu, spriteQueue.instances, sizeof(QuadSSBO) * solidCount);
    
    // Copy FX sprites starting at slot `solidCount`
    u32 fxCount = spriteFxQueue.count;
    memcpy(gpu + solidCount, spriteFxQueue.instances, sizeof(QuadSSBO) * fxCount);
    
    VkCommandBuffer cmd = Command_Buffer_Get();
    
    // Draw solid sprites
    Bind_Pipeline(cmd, PIPE_SPRITE);
    vkCmdDraw(cmd, 6, solidCount, 0, 0);
    
    // Draw FX sprites, using firstInstance to start at slot solidCount
    Bind_Pipeline(cmd, PIPE_SPRITE_FX);
    vkCmdDraw(cmd, 6, fxCount, 0, solidCount);   // ← firstInstance = solidCount
    
    spriteQueue.count = 0;
    spriteFxQueue.count = 0;
}