#include "sol_core.h"
#include <cglm/cglm.h>

void Sol_System_Model_Draw(World *world, double dt, double time)
{
    ModelInstanceData *gpuData = (ModelInstanceData *)instanceDataPtr[currentFrame];
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
        uint32_t handle = world->models[id].gpuHandle;
        uint32_t slot = cursors[handle]++;

        mat4 m;
        glm_mat4_identity(m);
        glm_translate(m, (vec3){xform->pos.x, xform->pos.y, xform->pos.z});

        mat4 rotM;
        versor q = {xform->rot.x, xform->rot.y, xform->rot.z, xform->rot.w};
        if (glm_quat_norm(q) < 0.1f)
            glm_quat_identity(q);
        glm_quat_mat4(q, rotM);
        glm_mat4_mul(m, rotM, m);

        vec3 scale = {1, 1, 1};
        scale[0] = xform->scale.x ? xform->scale.x : 1.0f;
        scale[1] = xform->scale.y ? xform->scale.y : 1.0f;
        scale[2] = xform->scale.z ? xform->scale.z : 1.0f;
        glm_scale(m, scale);

        memcpy(gpuData[slot].modelMatrix, m, sizeof(mat4));
        gpuData[slot].color[0] = 1.0f;
        gpuData[slot].color[1] = 1.0f;
        gpuData[slot].color[2] = 1.0f;
        gpuData[slot].color[3] = 1.0f;
    }
    Sol_Begin_3D();
    for (int b = 0; b < SOL_MODEL_COUNT; b++)
    {
        if (counts[b] > 0)
            Sol_Draw_Model_Instanced(b, counts[b], offsets[b]);
    }
}