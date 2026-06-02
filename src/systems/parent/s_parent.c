#include "sol_core.h"

void Sol_Parent_Init(World *world)
{
    world->parents  = calloc(MAX_ENTS, sizeof(CompParent));
    WAddStep(world) = Sol_Parent_Step;
}

void Sol_Parent_Add(World *world, int id, CompParent desc)
{
    world->parents[id] = desc;
    world->masks[id] |= HAS_PARENT;
}

void Sol_Parent_Step(World *world, double dt, double time)
{
    int required = HAS_PARENT;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompParent *parent = &world->parents[id];
        CompXform  *xform  = &world->xforms[id];

        xform->pos  = vecAdd(world->xforms[parent->parentId].pos, parent->localOffset);
        xform->quat = glms_quat_mul(parent->localQuat, world->xforms[parent->parentId].quat);
    }
}

u32 Sol_Parent_GetParent(World *world, int id)
{
    return world->parents[id].parentId;
}