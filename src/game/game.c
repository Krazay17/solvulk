#include "game.h"
#include "sol_core.h"
#include "world.h"
#include "game/prefabs.h"

static int dude;

static void Debug(World *world, double dt)
{
    // Sol_Anim_Play(solState.worlds[0], dude, (AnimDesc){.anim = ANIM_MANTLE_ROLL, .layerId = ANIM_LAYER_OVERRIDE});
}

void Create_Sol_Game()
{
    World *world            = World_Create();
    user_session.user_world = 0;

    Sol_Sys_Add(world, WORLDSYS_CONTROLLER);
    Sol_Sys_Add(world, WORLDSYS_MOVE3);
    Sol_Sys_Add(world, WORLDSYS_BODY3);
    Sol_Sys_Add(world, WORLDSYS_CAMERA);
    Sol_Sys_Add(world, WORLDSYS_ANIM);
    Sol_Sys_Add(world, WORLDSYS_MODEL);
    WAddTick(world) = Debug;

    dude                    = Sol_Prefab_Dude(world, (vec3s){0, 2, 0}, 1.0f);
    user_session.user_entid = dude;
    Sol_Comp_Add(world, dude, SolController);
    Sol_Comp_Add(world, dude, SolPlayer);
    SolCamera *camera     = Sol_Comp_Add(world, dude, SolCamera);
    SolMove3  *move       = Sol_Comp_Add(world, dude, SolMove3);
    camera->fov           = 80.0f;
    camera->up.y          = 1.0f;
    camera->lerpspeed     = 20.0f;
    camera->target_offset = 1.0f;
    move->kind            = MOVEMENTKIND_PLAYER;

    Sol_Prefab_Dude(world, (vec3s){2.0f, 1, 0}, 1.0f);
}