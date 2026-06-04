#include "sol_core.h"

void Sol_Parent_Init(World *world)
{
    world->parents  = calloc(MAX_ENTS, sizeof(CompParent));
    WAddStep(world) = Sol_Parent_Step;
}

CompParent *Sol_Parent_Add(World *world, int id, int parentId)
{
    world->parents[id].parentId = parentId;
    world->parents[id].active   = true;
    world->masks[id] |= HAS_PARENT;
    return &world->parents[id];
}

void Sol_Parent_Set(World *world, int id, CompParent desc)
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
        if (!parent->active)
            continue;

        xform->pos  = vecAdd(world->xforms[parent->parentId].pos, parent->localOffset);
        xform->quat = glms_quat_mul(parent->localQuat, world->xforms[parent->parentId].quat);
    }
}

u32 Sol_Parent_GetParent(World *world, int id)
{
    return world->parents[id].parentId;
}
void Sol_Parent_SetActive(World *world, int id, bool active)
{
    world->parents[id].active = active;
}
void Sol_Parent_SetOffset(World *world, int id)
{
    vec3s parentPos                = Sol_Xform_GetPos(world, world->parents[id].parentId);
    vec3s currentPos               = Sol_Xform_GetPos(world, id);
    vec3s offset                   = glms_vec3_sub(currentPos, parentPos);
    world->parents[id].localOffset = offset;
}
void Sol_Parent_SetWithOffset(World *world, int id, int parent)
{
    vec3s parentPos                = Sol_Xform_GetPos(world, parent);
    vec3s currentPos               = Sol_Xform_GetPos(world, id);
    vec3s offset                   = glms_vec3_sub(currentPos, parentPos);
    world->parents[id].parentId    = parent;
    world->parents[id].localOffset = offset;
}
