#include "prefabs.h"
#include "world.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    SolModel *model = Sol_Comp_Add(world, id, SolModel);
    model->modelId  = MODELKIND_DUDE;
    Sol_Anim_Add(world, id);
    Sol_Body3_Add(world, id);

    return id;
}