/*
 * File: s_parent.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-30
 *
 */
#include "world.h"
#include "sol_math.h"

void Parent_Update(World *world, double dt)
{
    SparseSet_ScParent *set = Sol_Comp_Set(world, ScParent);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScParent *parent = &set->data[i];

        Xform xform_parent = Xform_GetDraw(world, parent->parentId);
        if (parent->boneFollow)
        {
            xform_parent = Sol_Model_GetBoneXform(world, parent->parentId, parent->boneFollow);
        }

        vec3s offset    = glms_quat_rotatev(xform_parent.rot, parent->localOffset);
        vec3s pos_final = glms_vec3_add(xform_parent.pos, offset);

        versors rot_final = glms_quat_mul(xform_parent.rot, parent->localQuat);
        vec3s sca_final   = glms_vec3_mul(xform_parent.sca, parent->localScale);

        Xform_SetAll(world, id, pos_final, rot_final, sca_final);
    }
}

int Sol_Parent_GetChildren(World *world, int id, int *ids, int max_ids)
{
    SparseSet_ScParent *set = Sol_Comp_Set(world, ScParent);
    int child_count         = 0;
    for (int i = 0; i < set->cnt; i++)
    {
        int child_id     = set->dense[i];
        ScParent *parent = &set->data[i];
        if (child_count >= max_ids)
            break;
        if (parent->parentId == id)
            ids[child_count++] = child_id;
    }
    return child_count;
}