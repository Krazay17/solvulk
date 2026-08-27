#include "game.h"
#include "sol_core.h"
#include "world.h"
#include "game/prefabs.h"

static int dude1;

static void Debug(World *world, double dt, double time)
{
    Sol_Anim_Play(solState.worlds[0], dude1, (AnimDesc){.anim = ANIM_MANTLE_ROLL, .layerId = ANIM_LAYER_OVERRIDE});
}

void Create_Sol_Game()
{
    World *world = World_Create();

    WAddTick(world) = Debug;
    WAddTick(world) = Anim_Tick;
    WAdd3d(world)   = Model_Draw;

    dude1 = Sol_Prefab_Dude(world, (vec3s){0, 0, 0}, 1.0f);
    Sol_Prefab_Dude(world, (vec3s){2.0f, 0, 0}, 1.0f);
}