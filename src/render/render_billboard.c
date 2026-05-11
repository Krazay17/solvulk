#include "render.h"
#include "render_internal.h"

#include "render/vk/vkrender.h"

typedef struct
{
    u32           count;
    BillboardSSBO instances[MAX_BILLBOARD_INSTANCES];
    FlagsSSBO     flags[MAX_BILLBOARD_INSTANCES];
} BillboardSubmission;

static BillboardSubmission billboardQueue;

void Sol_Render_PushBillboard(BillboardDesc desc)
{
    if (billboardQueue.count >= MAX_BILLBOARD_INSTANCES)
        return;

    u32            idx = billboardQueue.count++;
    BillboardSSBO *gpu = &billboardQueue.instances[idx];

    memcpy(gpu->pos, desc.pos.raw, sizeof(vec4));
    memcpy(gpu->params, desc.params.raw, sizeof(vec4));

    gpu->color[0] = ColorConvert(desc.color.r);
    gpu->color[1] = ColorConvert(desc.color.g);
    gpu->color[2] = ColorConvert(desc.color.b);
    gpu->color[3] = ColorConvert(desc.color.a);
    gpu->type     = (u32)desc.kind;

    billboardQueue.flags[idx].flags = desc.flags;
}

void Flush_Billboards()
{
    BillboardSSBO *gpu = Sol_GetDescriptorMapping(DESC_BILLBOARD_SSBO);
    memcpy(gpu, billboardQueue.instances, sizeof(BillboardSSBO) * billboardQueue.count);

    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_BILLBOARD);
    vkCmdDraw(cmd, 6, billboardQueue.count, 0, 0);

    billboardQueue.count = 0;
}

// BILLBOARD_HEALTHBAR: params.x = fill (0..1), params.y = border thickness (0 = default 0.05)
// Sol_Render_PushBillboard((BillboardDesc){
//     .kind  = BILLBOARD_HEALTHBAR,
//     .pos   = aboveHead,
//     .size  = 1.5f,
//     .color = (vec4s){{0.2f, 0.8f, 0.2f, 1.0f}},
//     .params = (vec4s){{vital->hp / (float)vital->hpMax, 0.05f, 0, 0}},
// });