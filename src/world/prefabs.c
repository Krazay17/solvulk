#include "sol_core.h"

int Sol_Prefab_Wizard(World *world, float pos[])
{
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos=*pos});
    Entity_Add_Body(world, id, (CompBody){.height=1,.width=0.5f,.mass=1});
}

int Sol_Prefab_Button(World *world, float pos[])
{
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos=*pos});
    Entity_Add_Shape(world, id, (CompShape){.type=SHAPE_RECTANGLE,.height=50, .width=150});
    Entity_Add_Interact(world, id, (CompInteractable){0});
}
