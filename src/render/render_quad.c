#include "sol_core.h"

#include "render_i.h"

#include "render/vk/vkrender.h"

typedef struct
{
    u32      count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} QuadQueue;

static QuadQueue quadQueue;

void Sol_Render_PushQuad(QuadDesc desc)
{
    u32       idx  = quadQueue.count++;
    QuadSSBO *ssbo = &quadQueue.instances[idx];
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

void Flush_Quads(void)
{
    QuadSSBO *gpu = Sol_GetDescriptorMapping(DESC_QUAD);
    memcpy(gpu, quadQueue.instances, sizeof(QuadSSBO) * quadQueue.count);
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_QUAD);
    vkCmdDraw(cmd, 6, quadQueue.count, 0, 0);
    quadQueue.count = 0;
}