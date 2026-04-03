#include <cglm/cglm.h>

#include "sol_core.h"

void Sol_System_Model_Draw(World *world, double dt, double time)
{
    // shader data
    ModelSSBO *gpuData = (ModelSSBO *)Sol_ModelBuffer_Get();

    int required = HAS_XFORM | HAS_MODEL;

    // 1. Count per model
    uint32_t counts[SOL_MODEL_COUNT] = {0};
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
            counts[world->models[id].gpuHandle]++;
    }

    // 2. Prefix sum → offsets
    uint32_t offsets[SOL_MODEL_COUNT] = {0};
    for (int i = 1; i < SOL_MODEL_COUNT; i++)
        offsets[i] = offsets[i - 1] + counts[i - 1];

    // 3. Fill SSBO (cursor tracks next write slot per model)
    uint32_t cursors[SOL_MODEL_COUNT];
    memcpy(cursors, offsets, sizeof(offsets));

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *xform = &world->xforms[id];
        uint32_t slot = cursors[world->models[id].gpuHandle]++;

        ModelSSBO *inst = &gpuData[slot];

        memcpy(inst->position, &xform->pos, sizeof(float) * 3);
        inst->position[3] = xform->scale.x;

        memcpy(inst->rotation, &xform->rot, sizeof(float) * 4);

        // vec4 whiteColor = {1.0f, 1.0f, 1.0f, 1.0f};
        // memcpy(inst->color, &whiteColor, sizeof(float) * 4);
        // memcpy(inst->material, &whiteColor, sizeof(float) * 4);
    }

    Sol_Begin_3D();
    for (int b = 0; b < SOL_MODEL_COUNT; b++)
    {
        if (counts[b] > 0)
            Sol_Draw_Model_Instanced(b, counts[b], offsets[b]);
    }
}