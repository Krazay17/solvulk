/*
 * File: game.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-03
 *
 */

#include "game.h"
#include "game/prefabs.h"

static void PressTest(World *world, double dt, int id, void *data)
{
    ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
    if (body)
        body->vel.y += 50.0f;
}

static void Debug(World *world, double dt)
{
    World *game_world = Sol_User_GetGameWorld();
    if (!game_world)
        return;

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int       id     = player_set->dense[i];
        ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
        combat->health   = fmodf(combat->health + sin(dt * 100.0f), 100.0f);
    }
}

static void Healthbar_Hook(World *w, double dt, int id, void *data)
{
    World   *game_world = Sol_User_GetGameWorld();
    ScView2 *view2      = Sol_Comp_Get(w, id, ScView2);
    if (!game_world || !view2)
        return;

    float totalHealth    = 0.0f;
    float totalMaxHealth = 0.0f;

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int       id     = player_set->dense[i];
        ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
        if (!combat)
            continue;
        totalHealth += combat->health;
        totalMaxHealth += combat->maxHealth;
    }
    if (totalMaxHealth > 0.0f)
        view2->views[0].targetFill = clamp(totalHealth / totalMaxHealth, 0.0f, 1.0f);
    else
        view2->views[0].targetFill = 0.0f;
}

void Create_Sol_Game()
{
    Create_Menu();
    Create_Hud();
    Create_Game();
}

void Create_Menu()
{
    World *world        = World_Create();
    sol_user.menu_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);

    {
        int id = Sol_Create_Ent(world);
        Sol_Xform_Add(world, id, (vec3s){400, 400, 0});
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->update  = Healthbar_Hook;
        ScBody2 *body = Sol_Comp_Add(world, id, ScBody2);
        *body         = (ScBody2){
            .shape       = SHAPE2_REC,
            .restitution = 1.0f,
            .dims        = {100.0f, 100.0f, 0},
            .gravity     = {0, 98.1f, 0},
        };
        ScView2 *view  = Sol_Comp_Add(world, id, ScView2);
        view->count    = 1;
        view->views[0] = (View2){
            .kind       = VIEW2DKIND_RECT,
            .dims       = {100.0f, 100.0f},
            .color      = {1, 0, 0, 1},
            .scale      = 1.0f,
            .fill       = 1.0f,
            .targetFill = 1.0f,
            .hoverColor = {1, 1, 1, 1},
        };
    }
}

void Create_Hud()
{
    World *world       = World_Create();
    sol_user.hud_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);

    Sol_Prefab_Crosshair(world);
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;
    WAddPosttick(world) = Debug;

    {
        int id = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
        Sol_Debug_Add("Player Ent", id);
        Sol_Comp_Add(world, id, ScCmd);
        Sol_Comp_Add(world, id, ScPlayer);
        Sol_Comp_Add(world, id, ScCombat)->maxHealth = 100.0f;
        ScCamera *camera                             = Sol_Comp_Add(world, id, ScCamera);
        camera->fov                                  = 80.0f;
        camera->up.y                                 = 1.0f;
        camera->lerpspeed                            = 10.0f;
        camera->desired_offset                       = 1.0f;
        camera->desired_distance                     = 2.0f;
        sol_user.view_ent                            = id;
    }

    {
        int level1 = Sol_Create_Ent(world);
        Sol_Xform_Add(world, level1, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }

    while (world->entCount < 500)
    {
        int id = Sol_Create_Ent(world);
        Sol_Xform_Add(world, id, (vec3s){sinf(id) * 10.0f, 50.0f, cosf(id) * 10.0f});
        Sol_Comp_Add(world, id, ScCmd);
        Sol_Comp_Add(world, id, ScPlayer);
        // Sol_Comp_Add(world, id, ScCombat);

        ScInteract *interact                         = Sol_Comp_Add(world, id, ScInteract);
        interact->range                              = 5.0f;
        Sol_Comp_Add(world, id, ScHook)->pressed     = PressTest;
        Sol_Comp_Add(world, id, ScCombat)->maxHealth = 100.0f;
        Sol_Comp_Add(world, id, ScMove3);
        ScModel *model = Sol_Comp_Add(world, id, ScModel);
        ScBody3 *body3 = Sol_Body3_Add(world, id);
        body3->shape   = SHAPE3_CAP;
        body3->mask    = PHYSXMASK(1, 1);
        body3->dims    = (vec3s){0.5f, 3.0f, 0.5f};
        model->kind    = MODELKIND_EVANRIGGED;
        Sol_Anim_Add(world, id);
    }
}
