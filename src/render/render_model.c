#include "sol_core.h"

typedef struct
{
    u32        count;
    ModelSSBO  modelSSBO[MAX_MODEL_INSTANCES];
    FlagsSSBO  flags[MAX_MODEL_INSTANCES];
    SolModelId handles[MAX_MODEL_INSTANCES];
} ModelSubmission;

typedef struct
{
    u32        count;
    ModelSSBO  modelSSBO[MAX_MODEL_INSTANCES];
    FlagsSSBO  flags[MAX_MODEL_INSTANCES];
    BonesSSBO  bones[MAX_MODEL_INSTANCES];
    SolModelId handles[MAX_MODEL_INSTANCES];
} ModelSkinnedSubmission;

static u32                    model_que_offset;
static ModelSubmission        modelQueue;
static ModelSkinnedSubmission skinningQueue;

void Sol_Render_PushModel(ModelPushDesc desc)
{
    if (desc.hasAnim)
    {
        if (skinningQueue.count >= MAX_MODEL_INSTANCES)
            return;

        u32 slot                    = skinningQueue.count++;
        skinningQueue.handles[slot] = desc.handle;
        memcpy(&skinningQueue.modelSSBO[slot].position, &desc.position, sizeof(vec4));
        memcpy(&skinningQueue.modelSSBO[slot].rotation, &desc.rotation, sizeof(vec4));
        memcpy(&skinningQueue.modelSSBO[slot].scale, &desc.scale, sizeof(vec4));
        memcpy(&skinningQueue.modelSSBO[slot].color, &desc.color, sizeof(vec4));
        memcpy(&skinningQueue.modelSSBO[slot].material, &desc.material, sizeof(vec4));
        memcpy(&skinningQueue.bones[slot].bones, desc.bones, sizeof(mat4) * MAX_BONES);

        skinningQueue.flags[slot].flags = desc.flags;
    }
    else
    {
        u32 slot                 = modelQueue.count++;
        modelQueue.handles[slot] = desc.handle;
        memcpy(&modelQueue.modelSSBO[slot].position, &desc.position, sizeof(vec4));
        memcpy(&modelQueue.modelSSBO[slot].rotation, &desc.rotation, sizeof(vec4));
        memcpy(&modelQueue.modelSSBO[slot].scale, &desc.scale, sizeof(vec4));
        memcpy(&modelQueue.modelSSBO[slot].color, &desc.color, sizeof(vec4));
        memcpy(&modelQueue.modelSSBO[slot].material, &desc.material, sizeof(vec4));

        modelQueue.flags[slot].flags = desc.flags;
    }
}

void Flush_Models(void)
{
    model_que_offset = 0;

    if (modelQueue.count == 0)
        return;

    // Count per handle
    uint32_t counts[SOL_MODEL_COUNT] = {0};
    for (int i = 0; i < modelQueue.count; i++)
        counts[modelQueue.handles[i]]++;

    // Prefix sum
    uint32_t offsets[SOL_MODEL_COUNT] = {0};
    for (int i = 1; i < SOL_MODEL_COUNT; i++)
        offsets[i] = offsets[i - 1] + counts[i - 1];

    // Write sorted into SSBO
    ModelSSBO *gpu = Sol_GetDescriptorMapping(DESC_MODEL_SSBO);
    FlagsSSBO *f   = Sol_GetDescriptorMapping(DESC_FLAGS_SSBO);

    uint32_t cursors[SOL_MODEL_COUNT];
    memcpy(cursors, offsets, sizeof(offsets));

    for (int i = 0; i < modelQueue.count; i++)
    {
        SolModelId h    = modelQueue.handles[i];
        gpu[cursors[h]] = modelQueue.modelSSBO[i];
        f[cursors[h]]   = modelQueue.flags[i];
        cursors[h]++;
    }

    for (int h = 0; h < SOL_MODEL_COUNT; h++)
    {
        if (counts[h] > 0)
        {
            Render_Model(h, counts[h], offsets[h]);
        }
    }

    model_que_offset += modelQueue.count;
    modelQueue.count = 0;
}

void Flush_Models_Skinned(void)
{
    if (skinningQueue.count == 0)
        return;

    // Count per handle
    uint32_t counts[SOL_MODEL_COUNT] = {0};
    for (int i = 0; i < skinningQueue.count; i++)
        counts[skinningQueue.handles[i]]++;

    // Prefix sum
    uint32_t offsets[SOL_MODEL_COUNT] = {0};
    for (int i = 1; i < SOL_MODEL_COUNT; i++)
        offsets[i] = offsets[i - 1] + counts[i - 1];

    // Write sorted into SSBO
    ModelSSBO *modelGpu = Sol_GetDescriptorMapping(DESC_MODEL_SSBO);
    BonesSSBO *boneGpu  = Sol_GetDescriptorMapping(DESC_SKINNING_SSBO);
    FlagsSSBO *flagGpu  = Sol_GetDescriptorMapping(DESC_FLAGS_SSBO);

    uint32_t cursors[SOL_MODEL_COUNT];
    memcpy(cursors, offsets, sizeof(offsets));

    for (int i = 0; i < skinningQueue.count; i++)
    {
        SolModelId h = skinningQueue.handles[i];

        // GLOBAL INDEX = Base Offset + Local Sorted Position
        uint32_t globalIdx = model_que_offset + cursors[h];

        // 1. Write model data to the global slot
        modelGpu[globalIdx] = skinningQueue.modelSSBO[i];

        // 2. Write flags to the global slot (Don't overwrite the static ones at index 0!)
        flagGpu[globalIdx] = skinningQueue.flags[i];

        // 3. Write bones to the global slot
        // This ensures shader's gl_InstanceIndex points to the right matrices
        boneGpu[globalIdx] = skinningQueue.bones[i];

        cursors[h]++;
    }

    for (int h = 0; h < SOL_MODEL_COUNT; h++)
    {
        if (counts[h] > 0)
        {
            Render_Model_Skinned(h, counts[h], model_que_offset + offsets[h]);
        }
    }

    model_que_offset += skinningQueue.count;
    skinningQueue.count = 0;
}