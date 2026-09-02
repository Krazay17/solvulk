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
    World *world            = World_Create();
    user_session.user_world = 0;

    Sol_Sys_Add(world, WORLDSYS_CONTROLLER);
    Sol_Sys_Add(world, WORLDSYS_MOVE3);
    Sol_Sys_Add(world, WORLDSYS_PHYSX);
    Sol_Sys_Add(world, WORLDSYS_CAMERA);
    Sol_Sys_Add(world, WORLDSYS_ANIM);
    Sol_Sys_Add(world, WORLDSYS_MODEL);
    Sol_Sys_Add(world, WORLDSYS_DEBUG);

    dude                    = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
    user_session.user_entid = dude;
    Sol_Debug_Add("Player Ent", dude);
    Sol_Comp_Add(world, dude, ScController);
    ScCamera *camera      = Sol_Comp_Add(world, dude, ScCamera);
    ScMove3  *move        = Sol_Comp_Add(world, dude, ScMove3);
    camera->fov           = 80.0f;
    camera->up.y          = 1.0f;
    camera->lerpspeed     = 20.0f;
    camera->target_offset = 1.0f;
    move->kind            = MOVEMENTKIND_PLAYER;

    int level1 = Sol_Create_Ent(world);
    Sol_Xform_Add(world, level1, (vec3s){0, 0, 0});
    ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
    levelModel->kind    = SOL_MODEL_WORLD10;
    ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
    stage->isDirty      = true;

    while (world->entCount < 100)
    {
        int id = Sol_Create_Ent(world);
        Sol_Xform_Add(world, id, (vec3s){sinf(id) * 10.0f, 50.0f, cosf(id) * 10.0f});
        ScModel *model = Sol_Comp_Add(world, id, ScModel);
        ScBody3 *body3 = Sol_Body3_Add(world, id);
        body3->shape   = SHAPE3_CAP;
        body3->mask    = PHYSXMASK(1, 1);
        body3->dims    = (vec3s){0.5f, 1.0f, 0.5f};
        model->kind    = MODELKIND_WIZARD;
        Sol_Anim_Add(world, id);
        SolLine *line  = Sol_Line_New(world);
        if (line)
        {
            line->a   = (vec3s){sinf(id) * 10.0f, 50.0f, cosf(id) * 10.0f};
            line->b   = (vec3s){0, 0, 0};
            line->aColor = VEC4_RED;
            line->bColor = VEC4_WHITE;
            line->ttl = 25.0f;
        }
    }
}