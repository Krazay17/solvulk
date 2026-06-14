#include "sol_core.h"
#include "render_i.h"

#include "render/vk/vkrender.h"

ModelSubmission        modelQueue;
ModelSkinnedSubmission skinningQueue;

SphereQueue   sphereQueue;
SphereQueue   sphereFxQueue;
FireballQueue fireballQueue;
RibbonQueue   ribbonQueue;
RibbonQueue   ribbonQueueFront;

QuadQueue healthQueue;
QuadQueue spriteQueue0;
QuadQueue spriteQueue1;
QuadQueue spriteQueueFront;
QuadQueue text3dQueue;

RectInstance rectQueue;
FontInstance font2dQueue;

void Flush_Models(void)
{
    // Always reset the frame allocation tracker to 0 at the start of flushing
    u32 model_que_offset = 0;

    // Early exit only if both queues are empty
    if (modelQueue.count == 0 && skinningQueue.count == 0)
        return;

    ModelSSBO *modelGpu = Sol_GetDescriptorMapping(DESC_MODEL_SSBO);

    // ==========================================
    // 1. STATIC MODELS PASS
    // ==========================================
    if (modelQueue.count > 0)
    {
        // Count per handle
        uint32_t counts[SOL_MODEL_COUNT] = {0};
        for (int i = 0; i < modelQueue.count; i++)
            counts[modelQueue.handles[i]]++;

        // Prefix sum
        uint32_t offsets[SOL_MODEL_COUNT] = {0};
        for (int i = 1; i < SOL_MODEL_COUNT; i++)
            offsets[i] = offsets[i - 1] + counts[i - 1];

        // Write sorted into SSBO
        uint32_t cursors[SOL_MODEL_COUNT];
        memcpy(cursors, offsets, sizeof(offsets));

        for (int i = 0; i < modelQueue.count; i++)
        {
            SolModelKind h = modelQueue.handles[i];
            // Since model_que_offset is 0 here, we map straight to the buffer start
            modelGpu[cursors[h]] = modelQueue.modelSSBO[i];
            cursors[h]++;
        }

        // Dispatch commands
        for (int h = 0; h < SOL_MODEL_COUNT; h++)
        {
            if (counts[h] > 0)
            {
                Render_Model(h, counts[h], offsets[h]);
            }
        }

        // Shift forward by static model footprint
        model_que_offset += modelQueue.count;
        modelQueue.count = 0;
    }

    // ==========================================
    // 2. SKINNED MODELS PASS
    // ==========================================
    if (skinningQueue.count > 0)
    {
        BonesSSBO *boneGpu = Sol_GetDescriptorMapping(DESC_SKINNING_SSBO);

        // Count per handle
        uint32_t counts[SOL_MODEL_COUNT] = {0};
        for (int i = 0; i < skinningQueue.count; i++)
            counts[skinningQueue.handles[i]]++;

        // Prefix sum (Local to skinning allocation space)
        uint32_t offsets[SOL_MODEL_COUNT] = {0};
        for (int i = 1; i < SOL_MODEL_COUNT; i++)
            offsets[i] = offsets[i - 1] + counts[i - 1];

        uint32_t cursors[SOL_MODEL_COUNT];
        memcpy(cursors, offsets, sizeof(offsets));

        for (int i = 0; i < skinningQueue.count; i++)
        {
            SolModelKind h = skinningQueue.handles[i];

            // GLOBAL INDEX = Current Queue Offset + Local Sorted Bucket Index
            uint32_t globalIdx = model_que_offset + cursors[h];

            // Write both model params and transformations to mirrored index offsets
            modelGpu[globalIdx] = skinningQueue.modelSSBO[i];
            boneGpu[globalIdx]  = skinningQueue.bones[i];

            cursors[h]++;
        }

        // Dispatch commands
        for (int h = 0; h < SOL_MODEL_COUNT; h++)
        {
            if (counts[h] > 0)
            {
                // Push draw call offset forward into the buffer using model_que_offset
                Render_Model_Skinned(h, counts[h], model_que_offset + offsets[h]);
            }
        }

        // Clean up allocation pointers
        model_que_offset += skinningQueue.count;
        skinningQueue.count = 0;
    }
}

void Flush_Spheres(void)
{
    SphereSSBO     *gpu = Sol_GetDescriptorMapping(DESC_SPHERE_SSBO);
    VkCommandBuffer cmd = Command_Buffer_Get();

    u32 currentOffset = 0;

    // 1. Regular Spheres
    u32 solidCount = sphereQueue.count;
    if (solidCount > 0)
    {
        memcpy(gpu + currentOffset, sphereQueue.instances, sizeof(SphereSSBO) * solidCount);
        Bind_Pipeline(cmd, PIPE_SPHERE);
        vkCmdDraw(cmd, 6, solidCount, 0, currentOffset);
        currentOffset += solidCount;
        sphereQueue.count = 0;
    }

    // 2. FX Spheres
    u32 fxCount = sphereFxQueue.count;
    if (fxCount > 0)
    {
        memcpy(gpu + currentOffset, sphereFxQueue.instances, sizeof(SphereSSBO) * fxCount);
        Bind_Pipeline(cmd, PIPE_SPHERE_FX);
        vkCmdDraw(cmd, 6, fxCount, 0, currentOffset);
        currentOffset += fxCount;
        sphereFxQueue.count = 0;
    }

    // 3. Fireballs
    u32 fireballCount = fireballQueue.count;
    if (fireballCount > 0)
    {
        memcpy(gpu + currentOffset, fireballQueue.instances, sizeof(SphereSSBO) * fireballCount);
        Bind_Pipeline(cmd, PIPE_FIREBALL);
        vkCmdDraw(cmd, 6, fireballCount, 0, currentOffset);
        currentOffset += fireballCount;
        fireballQueue.count = 0;
    }
}

void Flush_Quads()
{
    QuadSSBO       *gpu           = Sol_GetDescriptorMapping(DESC_QUAD_SSBO);
    VkCommandBuffer cmd           = Command_Buffer_Get();
    u32             currentOffset = 0;

    u32 healthCount = healthQueue.count;
    if (healthCount > 0)
    {
        memcpy(gpu + currentOffset, healthQueue.instances, sizeof(QuadSSBO) * healthCount);
        Bind_Pipeline(cmd, PIPE_HEALTHBAR);
        vkCmdDraw(cmd, 6, healthCount, 0, currentOffset);
        currentOffset += healthCount;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        healthQueue.count = 0;
    }

    u32 spriteCount0 = spriteQueue0.count;
    if (spriteCount0 > 0)
    {
        memcpy(gpu + currentOffset, spriteQueue0.instances, sizeof(QuadSSBO) * spriteCount0);
        Bind_Pipeline(cmd, PIPE_SPRITE);
        vkCmdDraw(cmd, 6, spriteCount0, 0, currentOffset);
        currentOffset += spriteCount0;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        spriteQueue0.count = 0;
    }

    u32 spriteCount1 = spriteQueue1.count;
    if (spriteCount1 > 0)
    {
        memcpy(gpu + currentOffset, spriteQueue1.instances, sizeof(QuadSSBO) * spriteCount1);
        Bind_Pipeline(cmd, PIPE_SPRITE_ADD);
        vkCmdDraw(cmd, 6, spriteCount1, 0, currentOffset);
        currentOffset += spriteCount1;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        spriteQueue1.count = 0;
    }

    if (spriteQueueFront.count > 0)
    {
        memcpy(gpu + currentOffset, spriteQueueFront.instances, sizeof(QuadSSBO) * spriteQueueFront.count);
        Bind_Pipeline(cmd, PIPE_SPRITE_FRONT);
        vkCmdDraw(cmd, 6, spriteQueueFront.count, 0, currentOffset);
        currentOffset += spriteQueueFront.count;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        spriteQueueFront.count = 0;
    }

    u32 textCount = text3dQueue.count;
    if (textCount > 0)
    {
        memcpy(gpu + currentOffset, text3dQueue.instances, sizeof(QuadSSBO) * textCount);
        Bind_Pipeline(cmd, PIPE_TEXT_3D);
        vkCmdDraw(cmd, 6, textCount, 0, currentOffset);
        currentOffset += textCount;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        text3dQueue.count = 0;
    }
}

void Flush_Rects()
{
    if (rectQueue.count == 0)
        return;
    RectSSBO       *gpu = Sol_GetDescriptorMapping(DESC_RECT_SSBO);
    VkCommandBuffer cmd = Command_Buffer_Get();

    memcpy(gpu, rectQueue.instances, sizeof(RectSSBO) * rectQueue.count);
    Bind_Pipeline(cmd, PIPE_RECTI);
    vkCmdDraw(cmd, 6, rectQueue.count, 0, 0);
    rectQueue.count = 0;
}

void Flush_Fonts2d()
{
    if (font2dQueue.count == 0)
        return;
    FontSSBO       *gpu = Sol_GetDescriptorMapping(DESC_FONT_SSBO);
    VkCommandBuffer cmd = Command_Buffer_Get();

    memcpy(gpu, font2dQueue.instances, sizeof(FontSSBO) * font2dQueue.count);
    Bind_Pipeline(cmd, PIPE_TEXT_2D);
    vkCmdDraw(cmd, 6, font2dQueue.count, 0, 0);
    font2dQueue.count = 0;
}

void Flush_Ribbons()
{
    if (ribbonQueue.count == 0 && ribbonQueueFront.count == 0)
        return;
    RibbonSegSSBO  *gpu           = Sol_GetDescriptorMapping(DESC_RIBBON_SSBO);
    VkCommandBuffer cmd           = Command_Buffer_Get();
    u32             currentOffset = 0;

    u32 regularCount = ribbonQueue.count;
    if (regularCount > 0)
    {
        memcpy(gpu + currentOffset, ribbonQueue.instances, sizeof(RibbonSegSSBO) * regularCount);
        Bind_Pipeline(cmd, PIPE_RIBBON);

        vkCmdDraw(cmd, 6, regularCount, 0, currentOffset);
        currentOffset += regularCount;

        ribbonQueue.count = 0;
    }

    u32 frontCount = ribbonQueueFront.count;
    if (frontCount > 0)
    {
        memcpy(gpu + currentOffset, ribbonQueueFront.instances, sizeof(RibbonSegSSBO) * frontCount);
        Bind_Pipeline(cmd, PIPE_RIBBON_FRONT);
        vkCmdDraw(cmd, 6, frontCount, 0, currentOffset);
        currentOffset += frontCount;
        ribbonQueueFront.count = 0;
    }
}