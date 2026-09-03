#include "game.h"
#include "sol_core.h"
#include "world.h"
#include "game/prefabs.h"

static int dude;

static void Debug(World *world, double dt)
{
}

void Create_Sol_Game()
{
    World *world        = World_Create();
    sol_user.user_world = 0;

    Sol_Sys_Add(world, WORLDSYS_PLAYER);
    Sol_Sys_Add(world, WORLDSYS_MOVE3);
    Sol_Sys_Add(world, WORLDSYS_PHYSX);
    Sol_Sys_Add(world, WORLDSYS_FACING);
    Sol_Sys_Add(world, WORLDSYS_CAMERA);
    Sol_Sys_Add(world, WORLDSYS_ANIM);
    Sol_Sys_Add(world, WORLDSYS_MODEL);
    Sol_Sys_Add(world, WORLDSYS_DEBUG);

    dude                = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
    sol_user.user_entid = dude;
    Sol_Debug_Add("Player Ent", dude);
    Sol_Comp_Add(world, dude, ScCmd);
    Sol_Comp_Add(world, dude, ScPlayer);
    ScCamera *camera         = Sol_Comp_Add(world, dude, ScCamera);
    camera->fov              = 80.0f;
    camera->up.y             = 1.0f;
    camera->lerpspeed        = 10.0f;
    camera->desired_offset   = 1.0f;
    camera->desired_distance = 2.0f;

    int level1 = Sol_Create_Ent(world);
    Sol_Xform_Add(world, level1, (vec3s){0, 0, 0});
    ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
    levelModel->kind    = SOL_MODEL_WORLD10;
    ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
    stage->isDirty      = true;

    while (world->entCount < 1000)
    {
        int id = Sol_Create_Ent(world);
        Sol_Xform_Add(world, id, (vec3s){sinf(id) * 10.0f, 50.0f, cosf(id) * 10.0f});
        Sol_Comp_Add(world, id, ScCmd);
        Sol_Comp_Add(world, id, ScPlayer);
        Sol_Comp_Add(world, id, ScMove3);
        ScModel *model = Sol_Comp_Add(world, id, ScModel);
        ScBody3 *body3 = Sol_Body3_Add(world, id);
        body3->shape   = SHAPE3_CAP;
        body3->mask    = PHYSXMASK(1, 1);
        body3->dims    = (vec3s){0.5f, 3.0f, 0.5f};
        model->kind    = MODELKIND_WIZARD;
        Sol_Anim_Add(world, id);
    }
}