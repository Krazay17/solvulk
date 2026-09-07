#include "world.h"
#include "sol_math.h"

#include <omp.h>

void Worlds_Xform_Snapshot(World **worlds, int count)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;
        WorldXform *xform = &world->xform;
        memcpy(xform->last_pos, xform->pos, sizeof(vec3s) * MAX_ENTS);
        memcpy(xform->last_sca, xform->sca, sizeof(vec3s) * MAX_ENTS);
        memcpy(xform->last_rot, xform->rot, sizeof(versors) * MAX_ENTS);
    }
}

void Worlds_Xform_Interpolate(World **worlds, int count, float alpha)
{
    int i;
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;

        WorldXform *xform = &world->xform;
#pragma omp parallel for
        for (i = 0; i < MAX_ENTS; i++)
        {
            xform->draw_pos[i] = glms_vec3_lerp(xform->last_pos[i], xform->pos[i], alpha);
            xform->draw_sca[i] = glms_vec3_lerp(xform->last_sca[i], xform->sca[i], alpha);
            xform->draw_rot[i] = glms_quat_nlerp(xform->last_rot[i], xform->rot[i], alpha);
        }
    }
}

void Sol_Xform_Teleport(World *world, int id, vec3s pos)
{
    world->xform.pos[id] = world->xform.last_pos[id] = world->xform.draw_pos[id] = pos;
}
