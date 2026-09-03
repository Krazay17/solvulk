/*
 * File: game.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-03
 *
 */

#include "game.h"
#include "game/prefabs.h"

static int dude;

static void Debug(World *world, double dt)
{
    World              *game_world = solState.worlds[1];
    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int       id     = player_set->dense[i];
        ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
        combat->health   = fmodf(combat->health + sin(dt * 100.0f), 100.0f);
    }
}

static void Healthbar_Interface(World *w, double dt, int id)
{
    World   *game_world = Sol_GetWorldByIdx(sol_user.game_world);
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
    totalHealth /= (float)player_set->cnt;
    totalMaxHealth /= (float)player_set->cnt;
    view2->views[0].targetFill = totalHealth / totalMaxHealth;
}

void Create_Sol_Game()
{
    World *menu         = World_Create();
    sol_user.menu_world = menu->index;
    Sol_Sys_Add(menu, WORLDSYS_HOOK);
    Sol_Sys_Add(menu, WORLDSYS_VIEW2);
    Sol_Prefab_Crosshair(menu);

    int healthbar = Sol_Create_Ent(menu);
    Sol_Xform_Add(menu, healthbar, (vec3s){400, 400, 0});
    ScHook *interf      = Sol_Comp_Add(menu, healthbar, ScHook);
    interf->update           = Healthbar_Interface;
    ScView2 *healthbarView2  = Sol_Comp_Add(menu, healthbar, ScView2);
    healthbarView2->count    = 1;
    healthbarView2->views[0] = (View2){
        .kind       = VIEW2DKIND_RECT,
        .dims       = {100.0f, 100.0f},
        .color      = {1, 0, 0, 1},
        .scale      = 1.0f,
        .fill       = 1.0f,
        .targetFill = 1.0f,
        .hoverColor = {1, 1, 1, 1},
    };

    World *world        = World_Create();
    sol_user.game_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_PLAYER);
    Sol_Sys_Add(world, WORLDSYS_MOVE3);
    Sol_Sys_Add(world, WORLDSYS_PHYSX);
    Sol_Sys_Add(world, WORLDSYS_FACING);
    Sol_Sys_Add(world, WORLDSYS_CAMERA);
    Sol_Sys_Add(world, WORLDSYS_ANIM);
    Sol_Sys_Add(world, WORLDSYS_MODEL);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);
    Sol_Sys_Add(world, WORLDSYS_DEBUG);
    WAddPosttick(world) = Debug;

    dude = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
    Sol_Debug_Add("Player Ent", dude);
    Sol_Comp_Add(world, dude, ScCmd);
    Sol_Comp_Add(world, dude, ScPlayer);
    Sol_Comp_Add(world, dude, ScCombat)->maxHealth = 100.0f;
    ScCamera *camera                               = Sol_Comp_Add(world, dude, ScCamera);
    camera->fov                                    = 80.0f;
    camera->up.y                                   = 1.0f;
    camera->lerpspeed                              = 10.0f;
    camera->desired_offset                         = 1.0f;
    camera->desired_distance                       = 2.0f;
    sol_user.view_ent                              = dude;

    int level1 = Sol_Create_Ent(world);
    Sol_Xform_Add(world, level1, (vec3s){0, 0, 0});
    ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
    levelModel->kind    = MODELKIND_WORLD10;
    ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
    stage->isDirty      = true;

    while (world->entCount < 100)
    {
        int id = Sol_Create_Ent(world);
        Sol_Xform_Add(world, id, (vec3s){sinf(id) * 10.0f, 50.0f, cosf(id) * 10.0f});
        Sol_Comp_Add(world, id, ScCmd);
        Sol_Comp_Add(world, id, ScPlayer);
        Sol_Comp_Add(world, id, ScCombat)->maxHealth = 100.0f;
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