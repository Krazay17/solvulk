#include "prefabs.h"
#include "world.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    model->kind    = MODELKIND_DUDE;
    Sol_Anim_Add(world, id);
    ScBody3 *body     = Sol_Body3_Add(world, id);
    body->restitution = 0.1f;
    body->invMass = 0.01f;
    body->shape       = SHAPE3_CAP;
    body->dims        = (vec3s){0.5f, 1.7f, 0.5f};
    body->mask        = PHYSXMASK(1, 1);

    return id;
}