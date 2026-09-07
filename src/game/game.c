/*
 * File: game.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-03
 *
 */
#include "game.h"

static void Debug(World *world, double dt)
{
    World *game_world = Sol_User_GetGameWorld();
    if (!game_world)
        return;

    // SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);
    // for (int i = 0; i < player_set->cnt; i++)
    // {
    //     int       id     = player_set->dense[i];
    //     ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
    //     combat->health   = fmodf(combat->health + sin(dt * 100.0f), 100.0f);
    // }
}

// ########################
// ####### PUBLIC #########
// ########################

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
    Sol_Sys_Add(world, WORLDSYS_INTERACT);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);

    { // QUIT BUTTON
        int id        = Sol_Prefab_Button(world, (vec3s){1130.0f, 100.0f, 0}, "QUIT", INTERACT_DRAGGABLE);
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->pressed = Hook_Quit;
        Sol_Comp_Get(world, id, ScView2)->layer = UILAYER_2;
    }

    { // FULLSCREEN BUTTON
        int id =
            Sol_Prefab_Button(world, (vec3s){0.0f, 400.0f, 0}, "FULLSCREEN", INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE);
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->pressed = Hook_Fullscreen;
    }
}

void Create_Hud()
{
    World *world       = World_Create();
    sol_user.hud_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_INTERACT);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);

    Sol_Prefab_Crosshair(world);

    // Healthbar
    {
        int id       = Sol_Prefab_Healthbar(world, (vec3s){400, 650, 0});
        ScHook *hook = Sol_Comp_Add(world, id, ScHook);
        hook->update = Hook_Healthbar;
    }
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;
    WAddPosttick(world) = Debug;

    // Player
    {
        int id            = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
        sol_user.view_ent = id;
        Sol_Debug_Add("Player Ent", id);
        Sol_Comp_Add(world, id, ScCmd);
        Sol_Comp_Add(world, id, ScPlayer);
        ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
        snprintf(meta->name, sizeof(meta->name), "Krazay");
        ScCamera *camera         = Sol_Comp_Add(world, id, ScCamera);
        camera->fov              = 80.0f;
        camera->up.y             = 1.0f;
        camera->lerpspeed        = 10.0f;
        camera->desired_offset   = 1.0f;
        camera->desired_distance = 2.0f;
    }

    // Level
    {
        int level1 = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }

    // Wizards
    while (world->entCount < 500)
    {
        int id = Sol_Create_Ent(world, (vec3s){sinf(id) * 10.0f, 50.0f, cosf(id) * 10.0f});
        // Sol_Comp_Add(world, id, ScCmd);
        // Sol_Comp_Add(world, id, ScPlayer);
        ScCombat *combat  = Sol_Comp_Add(world, id, ScCombat);
        combat->healthMax = 100.0f;
        combat->health    = 100.0f;
        ScMeta *meta      = Sol_Comp_Add(world, id, ScMeta);
        snprintf(meta->name, sizeof(meta->name), "Wizard %d", id);
        ScInteract *interact                     = Sol_Comp_Add(world, id, ScInteract);
        interact->range                          = 5.0f;
        Sol_Comp_Add(world, id, ScHook)->pressed = Hook_Test;
        ScModel *model                           = Sol_Comp_Add(world, id, ScModel);
        ScBody3 *body3                           = Sol_Body3_Add(world, id);
        body3->shape                             = SHAPE3_CAP;
        body3->mask                              = PHYSXMASK(1, 1);
        body3->dims                              = (vec3s){0.5f, 3.0f, 0.5f};
        model->kind                              = MODELKIND_WIZARD;
        Sol_Anim_Add(world, id);
    }
}
