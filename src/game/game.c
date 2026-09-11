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
        int id =
            Sol_Prefab_Button(world, (vec3s){1130.0f, 100.0f, 0}, "QUIT", INTERACT_DRAGGABLE, UILAYER_2, Hook_Quit);
    }
    { // DEBUG BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){1130.0f, 150.0f, 0}, "Debug",
                                   INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE, 0, Hook_DebugToggle);
    }
    { // FULLSCREEN BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){1130.0f, 200.0f, 0}, "FULLSCREEN",
                                   INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE, 0, Hook_Fullscreen);
    }
    { // WIZARD BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){0, 350.0f, 0}, "Wizard", INTERACT_DRAGGABLE, 0, Hook_SpawnWizard);
    }
    { // WIZARDS BUTTON
        int id       = Sol_Prefab_Button(world, (vec3s){0, 400.0f, 0}, "Wizards", INTERACT_DRAGGABLE, 0, NULL);
        ScHook *hook = Sol_Comp_Add(world, id, ScHook);
        hook->held   = Hook_SpawnWizard;
    }
    { // BUTTON WORLD1
        int id = Sol_Prefab_Button(world, (vec3s){0, 450.0f, 0}, "World1", INTERACT_DRAGGABLE, 0, Hook_SwitchWorld);
    }
    { // BUTTON WORLD2
        int id = Sol_Prefab_Button(world, (vec3s){0, 500.0f, 0}, "World2", INTERACT_DRAGGABLE, 0, Hook_SwitchWorld2);
    }
    {
        int id = Sol_Prefab_Slider(world, (vec3s){0.0f, 550.0f, 0}, "Volume", INTERACT_DRAGGABLE, 0, Hook_SetVolume);
    }
    Sol_Prefab_Button(world, (vec3s){0, 600.0f, 0}, "Clone", INTERACT_DRAGGABLE, 0, Hook_Clone);
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
     Sol_Prefab_AbilityCard(world, (vec3s){100.0f, 400.0f, 0}, ABILITY_STATE_CLAW);
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;
    WAddPosttick(world) = Debug;

    { // Player
        int id = Sol_Prefab_Dude(world, (vec3s){0, 6, 0}, 1.0f);
        Sol_Comp_Add(world, id, ScPlayer);
        ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
        snprintf(meta->name, sizeof(meta->name), "Krazay");
        sol_user.view_ent = id;
        Sol_Debug_Add("Player Ent", (float)id);

        int idB       = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScView3 *view = Sol_Comp_Add(world, idB, ScView3);
        view->kind    = VIEW3KIND_FIREBALL;
        view->color   = VEC4_RED;
        view->dims.x  = 0.5f;
        Sol_Comp_Add(world, idB, ScParent)->parentId = id;

        // int idB = Sol_Prefab_Dude(world, (vec3s){2, 6, 0}, 1.0f);
        // Sol_Comp_Add(world, idB, ScPlayer);
        // Sol_Comp_Add(world, idB, ScInteract);
        // Sol_Comp_Add(world, idB, ScHook)->release = Hook_Test;
    }
    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
    { // Drainable Crystal
        int id = Sol_Create_Ent(world, (vec3s){5, 5, 0});
        Sol_Comp_Add(world, id, ScInteract);
        Sol_Comp_Add(world, id, ScHook)->held = Hook_CrystalDrain;
    }

    // while (world->entCount < 4)
    // { // Wizards
    //     static int inc = 0;
    //     int id         = Sol_Prefab_Wizard(world, (vec3s){sinf(inc) * 10.0f, 50.0f, cosf(inc) * 10.0f}, 1.0f);
    //     Sol_Comp_Add(world, id, ScHook)->pressed = Hook_Test;
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
