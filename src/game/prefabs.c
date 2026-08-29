#include "prefabs.h"
#include "world.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    SolModel *model = Sol_Comp_Add(world, id, SolModel);
    model->kind  = MODELKIND_DUDE;
    Sol_Anim_Add(world, id);
    SolBody3 *body3= Sol_Body3_Add(world, id);
    body3->restitution = 0.01f;
    body3->shape = SHAPE3_SPH;
    body3->dims.x = 0.5f;
    body3->group = PHYSXMASK(1, 1);

    return id;
}