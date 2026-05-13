#include "sol_core.h"

#include "render_i.h"

#include "render/vk/vkrender.h"

typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_SPHERE_INSTANCES];
} SphereQueue;

static SphereQueue sphereQueue;

void Sol_Render_PushSphere(SphereDesc desc)
{
    u32         idx  = sphereQueue.count++;
    SphereSSBO *ssbo = &sphereQueue.instances[idx];
    memcpy(ssbo->pos, desc.pos.raw, sizeof(vec4));
    ssbo->color[0] = ColorConvert(desc.color.r);
    ssbo->color[1] = ColorConvert(desc.color.g);
    ssbo->color[2] = ColorConvert(desc.color.b);
    ssbo->color[3] = ColorConvert(desc.color.a);
}

void Flush_Spheres(void)
{
    SphereSSBO *gpu = Sol_GetDescriptorMapping(DESC_SPHERE);
    memcpy(gpu, sphereQueue.instances, sizeof(SphereSSBO) * sphereQueue.count);
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_SPHERE);
    vkCmdDraw(cmd, 6, sphereQueue.count, 0, 0);
    sphereQueue.count = 0;
}