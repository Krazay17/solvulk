#include "sol_core.h"

#include "render_i.h"

#include "render/vk/vkrender.h"

typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_SPHERE_INSTANCES];
} SphereQueue;

static SphereQueue sphereQueue;
static SphereQueue sphereFxQueue;

void Sol_Render_PushSphere(SphereDesc desc)
{
    SphereSSBO *ssbo;
    u32         idx;

    if (desc.isfx)
    {
        idx  = sphereFxQueue.count++;
        ssbo = &sphereFxQueue.instances[idx];
    }
    else
    {
        idx  = sphereQueue.count++;
        ssbo = &sphereQueue.instances[idx];
    }
    memcpy(ssbo->pos, desc.pos.raw, sizeof(vec4));
    ssbo->color[0] = desc.color.r;
    ssbo->color[1] = desc.color.g;
    ssbo->color[2] = desc.color.b;
    ssbo->color[3] = desc.color.a;
}

void Flush_Spheres(void)
{
    SphereSSBO *gpu = Sol_GetDescriptorMapping(DESC_SPHERE);
    
    u32 solidCount = sphereQueue.count;
    memcpy(gpu, sphereQueue.instances, sizeof(SphereSSBO) * solidCount);
    
    u32 fxCount = sphereFxQueue.count;
    memcpy(gpu + solidCount, sphereFxQueue.instances, sizeof(SphereSSBO) * fxCount);
    
    VkCommandBuffer cmd = Command_Buffer_Get();
    
    Bind_Pipeline(cmd, PIPE_SPHERE);
    vkCmdDraw(cmd, 6, solidCount, 0, 0);
    
    Bind_Pipeline(cmd, PIPE_SPHERE_FX);
    vkCmdDraw(cmd, 6, fxCount, 0, solidCount);
    
    sphereQueue.count = 0;
    sphereFxQueue.count = 0;
}