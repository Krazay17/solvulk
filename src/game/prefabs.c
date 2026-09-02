#include "prefabs.h"
#include "world.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    model->kind    = MODELKIND_EVAN;
    // Sol_Anim_Add(world, id);
    ScBody3 *body3     = Sol_Body3_Add(world, id);
    body3->restitution = 0.0f;
    body3->shape       = SHAPE3_CAP;
    body3->dims        = (vec3s){0.5f, 1.0f, 0.5f};
    body3->mask        = PHYSXMASK(1, 1);

    return id;
}