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
    Create_Game2();
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
    { // WIZARD BUTTON
        int id        = Sol_Prefab_Button(world, (vec3s){1130.0f, 200.0f, 0}, "Wizard", INTERACT_DRAGGABLE);
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->pressed = Hook_SpawnWizard;
    }
    { // WIZARDS BUTTON
        int id       = Sol_Prefab_Button(world, (vec3s){1130.0f, 250.0f, 0}, "Wizards", INTERACT_DRAGGABLE);
        ScHook *hook = Sol_Comp_Add(world, id, ScHook);
        hook->held   = Hook_SpawnWizard;
    }
    { // FULLSCREEN BUTTON
        int id =
            Sol_Prefab_Button(world, (vec3s){0.0f, 400.0f, 0}, "FULLSCREEN", INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE);
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->pressed = Hook_Fullscreen;
    }
    { // BUTTON WORLD1
        int id        = Sol_Prefab_Button(world, (vec3s){0, 500.0f, 0}, "World1", INTERACT_DRAGGABLE);
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->pressed = Hook_SwitchWorld;
    }
    { // BUTTON WORLD2
        int id        = Sol_Prefab_Button(world, (vec3s){0, 550.0f, 0}, "World2", INTERACT_DRAGGABLE);
        ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
        hook->pressed = Hook_SwitchWorld2;
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

    { // Player
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
    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }

    // while (world->entCount < 500)
    // { // Wizards
    //     static int inc = 0;
    //     int id         = Sol_Prefab_Wizard(world, (vec3s){sinf(inc) * 10.0f, 50.0f, cosf(inc) * 10.0f}, 1.0f);
    //     Sol_Comp_Add(world, id, ScHook)->pressed = Hook_Test;
    //     // Sol_Comp_Add(world, id, ScCmd);
    //     // Sol_Comp_Add(world, id, ScPlayer);
    //     inc++;
    // }
}

void Create_Game2()
{
    World *world        = World_Create_AllSys();
    world->doesSimulate = false;
    world->doesRender   = false;

    // { // Player
    //     int id = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
    //     Sol_Debug_Add("Player Ent", id);
    //     sol_user.view_ent = id;
    //     Sol_Comp_Add(world, id, ScCmd);
    //     Sol_Comp_Add(world, id, ScPlayer);
    //     ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
    //     snprintf(meta->name, sizeof(meta->name), "Krazay");
    //     ScCamera *camera         = Sol_Comp_Add(world, id, ScCamera);
    //     camera->fov              = 80.0f;
    //     camera->up.y             = 1.0f;
    //     camera->lerpspeed        = 10.0f;
    //     camera->desired_offset   = 1.0f;
    //     camera->desired_distance = 2.0f;
    // }
    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD1;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
}
