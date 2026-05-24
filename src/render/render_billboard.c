// #include "render_i.h"
// #include "sol_core.h"

// #include "render/vk/vkrender.h"

// typedef struct
// {
//     u32           count;
//     BillboardSSBO instances[MAX_QUAD_INSTANCES];
// } BillboardSubmission;

// static BillboardSubmission billboardQueue;

// void Sol_Render_PushBillboard(BillboardDesc desc)
// {
//     if (billboardQueue.count >= MAX_QUAD_INSTANCES)
//         return;

//     u32            idx = billboardQueue.count++;
//     BillboardSSBO *gpu = &billboardQueue.instances[idx];

//     memcpy(gpu->pos, desc.pos.raw, sizeof(vec4));
//     memcpy(gpu->params, desc.params.raw, sizeof(vec4));

//     gpu->color[0] = desc.color.r;
//     gpu->color[1] = desc.color.g;
//     gpu->color[2] = desc.color.b;
//     gpu->color[3] = desc.color.a;
//     gpu->type     = (u32)desc.kind;
//     gpu->flags    = desc.flags;
// }

// void Flush_Billboards()
// {
//     BillboardSSBO *gpu = Sol_GetDescriptorMapping(DESC_QUAD_SSBO);
//     memcpy(gpu, billboardQueue.instances, sizeof(BillboardSSBO) * billboardQueue.count);

//     VkCommandBuffer cmd = Command_Buffer_Get();
//     Bind_Pipeline(cmd, PIPE_QUAD);
//     vkCmdDraw(cmd, 6, billboardQueue.count, 0, 0);

//     billboardQueue.count = 0;
// }
