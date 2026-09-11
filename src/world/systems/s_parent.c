#include "world.h"
#include "sol_math.h"

void Parent_Update(World *world, double dt)
{
    SparseSet_ScParent *set = Sol_Comp_Set(world, ScParent);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScParent *parent = &set->data[i];
        if (!parent->active)
            continue;
        XformP xform       = Xform_GetP(world, id);
        Xform xform_parent = Xform_Get(world, parent->parentId);

        *xform.pos = glms_vec3_add(*xform.pos, glms_vec3_add(xform_parent.pos, parent->localOffset));
        *xform.rot = glms_quat_mul(*xform.rot, xform_parent.rot);
        *xform.sca = glms_vec3_mul(*xform.sca, xform_parent.sca);
    }
}
