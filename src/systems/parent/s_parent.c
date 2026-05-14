#include "sol_core.h"

void Sol_Parent_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Parent_Step;
}

void Sol_Parent_Add(World *world, int id, CompParent desc)
{
    world->masks[id] |= HAS_PARENT;
    CompParent *parent = &world->parents[id];
    memcpy(parent, &desc, sizeof(CompParent));
}

void Sol_Parent_Step(World *world, double dt, double time)
{
    int required = HAS_PARENT | HAS_XFORM;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompParent *parent = &world->parents[id];
        CompXform  *xform  = &world->xforms[id];

        xform->pos = vecAdd(parent->localOffset, world->xforms[parent->parentId].pos);
        xform->quat = glms_quat_add(parent->localQuat, world->xforms[parent->parentId].quat);
    }
}
