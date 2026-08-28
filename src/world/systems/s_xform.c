#include "world.h"
#include "sol_math.h"

void Worlds_Xform_Snapshot(World **worlds, int count)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;
        SparseSet_SolXform *xform_set = Sol_Comp_Set(world, SolXform);
        for (int i = 0; i < xform_set->cnt; i++)
        {
            SolXform *xform = &xform_set->data[i];
            xform->last_pos = xform->pos;
            xform->last_sca = xform->sca;
            xform->last_rot = xform->rot;
        }
    }
}

void Worlds_Xform_Interpolate(World **worlds, int count, float alpha)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;
        SparseSet_SolXform *xform_set = Sol_Comp_Set(world, SolXform);
        for (int i = 0; i < xform_set->cnt; i++)
        {
            SolXform *xform = &xform_set->data[i];
            xform->draw_pos = glms_vec3_lerp(xform->last_pos, xform->pos, alpha);
            xform->draw_sca = glms_vec3_lerp(xform->last_sca, xform->sca, alpha);
            xform->draw_rot = glms_quat_nlerp(xform->last_rot, xform->rot, alpha);
        }
    }
}

void Sol_Xform_Teleport(World *world, int id, vec3s pos)
{
    SolXform *xform = Sol_Comp_Get(world, id, SolXform);
    xform->pos = xform->last_pos = xform->draw_pos = pos;
}
