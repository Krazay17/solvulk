#include "s_parent.h"
#include "model/s_model.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "physx/s_body2d.h"

static void Parent_Tick(World *world, double dt, double time)
{
    int required = BITC(HAS_PARENT);
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompParent *parent = &world->parents[id];
        CompXform  *xform  = &world->xforms[id];
        if (!parent->active)
            continue;
        if (world->masks[id] & BITC(HAS_BODY2))
            world->body2d[id].vel = GLMS_VEC2_ZERO;

        if (parent->boneFollow[0] != 0)
        {
            SolXform boneXform = Sol_Model_GetBoneXform(world, parent->parentId, parent->boneFollow);
            Sol_Xform_SetXform(world, id, boneXform);
        }
        else
        {
            xform->pos  = vecAdd(world->xforms[parent->parentId].pos, parent->localOffset);
            xform->quat = glms_quat_mul(parent->localQuat, world->xforms[parent->parentId].quat);
        }
    }
}

void Sol_Parent_Init(World *world)
{
    world->parents = calloc(MAX_ENTS, sizeof(CompParent));

    WAddTick(world) = Parent_Tick;
}

CompParent *Sol_Parent_Add(World *world, int id, int parentId)
{
    CompParent *parent = &world->parents[id];
    parent->parentId   = parentId;
    parent->active     = true;
    world->masks[id] |= BITC(HAS_PARENT);
    return parent;
}

void Sol_Parent_Set(World *world, int id, CompParent desc)
{
    world->parents[id] = desc;
    world->masks[id] |= BITC(HAS_PARENT);
}

u32 Sol_Parent_GetParent(World *world, int id)
{
    if (world->masks[id] & BITC(HAS_PARENT) && world->parents[id].parentId)
        return world->parents[id].parentId;
    else
        return id;
}
void Sol_Parent_SetActive(World *world, int id, bool active)
{
    world->parents[id].active = active;
    world->masks[id] |= BITC(HAS_PARENT);
}
bool Sol_Parent_IsActive(World *world, int id)
{
    return world->parents[id].active;
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
