#include "world.h"
#include "sol_math.h"

void Parent_Update(World *world, double dt)
{
    SparseSet_ScParent *set = Sol_Comp_Set(world, ScParent);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScParent *parent = &set->data[i];

        Xform xform_parent = Xform_Get(world, parent->parentId);

        vec3s pos_final   = glms_vec3_add(xform_parent.pos, parent->localOffset);
        versors rot_final = glms_quat_mul(xform_parent.rot, parent->localQuat);
        vec3s sca_final   = xform_parent.sca;

        world->xform.pos[id] = pos_final;
        world->xform.rot[id] = rot_final;
        world->xform.sca[id] = sca_final;
    }
}
