/*
 * File: game.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-03
 *
 */
#include "game.h"

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
            Sol_Prefab_Button(world, (vec3s){1130.0f, 100.0f, 0}, "QUIT", INTERACT_DRAGGABLE, UILAYER_4, Hook_Quit);
    }
    { // DEBUG BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){1130.0f, 150.0f, 0}, "Debug",
                                   INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE, UILAYER_4, Hook_DebugToggle);
    }
    { // FULLSCREEN BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){1130.0f, 200.0f, 0}, "FULLSCREEN",
                                   INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE, UILAYER_4, Hook_Fullscreen);
    }
    { // WIZARD BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){0, 350.0f, 0}, "Wizard", INTERACT_DRAGGABLE, UILAYER_4, Hook_SpawnWizard);
    }
    { // WIZARDS BUTTON
        int id       = Sol_Prefab_Button(world, (vec3s){0, 400.0f, 0}, "Wizards", INTERACT_DRAGGABLE, UILAYER_4, NULL);
        ScHook *hook = Sol_Comp_Add(world, id, ScHook);
        hook->held   = Hook_SpawnWizard;
    }

    Sol_Prefab_Button(world, (vec3s){0, 450.0f, 0}, "World1", INTERACT_DRAGGABLE, UILAYER_4, Hook_SwitchWorld);
    Sol_Prefab_Button(world, (vec3s){0, 500.0f, 0}, "World2", INTERACT_DRAGGABLE, UILAYER_4, Hook_SwitchWorld2);
    Sol_Prefab_Slider(world, (vec3s){0.0f, 550.0f, 0}, "Volume", INTERACT_DRAGGABLE, UILAYER_4, Hook_SetVolume);
    Sol_Prefab_Button(world, (vec3s){0, 600.0f, 0}, "Clone", INTERACT_DRAGGABLE, UILAYER_4, Hook_Clone);
    Sol_Prefab_Button(world, (vec3s){150.0f, 350.0f, 0}, "AddItem", INTERACT_DRAGGABLE, UILAYER_4, Hook_AddItem);
    Sol_Prefab_Button(world, (vec3s){150.0f, 400.0f, 0}, "Save", INTERACT_DRAGGABLE, UILAYER_4, Hook_SaveUser);
    Sol_Prefab_Button(world, (vec3s){150.0f, 450.0f, 0}, "Save Clear", INTERACT_DRAGGABLE, UILAYER_4, Hook_SaveClear);
    Sol_Prefab_Button(world, (vec3s){150.0f, 500.0f, 0}, "Emitter", INTERACT_DRAGGABLE, UILAYER_4, Hook_SpawnEmitter);
    Sol_Prefab_Button(world, (vec3s){150.0f, 550.0f, 0}, "Emitter", INTERACT_DRAGGABLE, UILAYER_4, Hook_SpawnEmitter2);
    Sol_Prefab_Button(world, (vec3s){150.0f, 600.0f, 0}, "Spawn Player", INTERACT_DRAGGABLE, UILAYER_4, Hook_SpawnPlayer);
}

void Create_Hud()
{
    World *world       = World_Create();
    sol_user.hud_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_INTERACT);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);
    Sol_Sys_Add(world, WORLDSYS_ABILITYBAR);
    Sol_Comp_Add(world, 0, SlContacts2);

    Sol_Prefab_Crosshair(world);
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;

    { // Player
        int id = Sol_Prefab_Dude(world, (vec3s){0, 6, -5}, 1.0f);
        Sol_Comp_Add(world, id, ScPlayer);
        ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
        snprintf(meta->name, sizeof(meta->name), "Player");
        sol_user.view_ent = id;
        Sol_Debug_Add("Player Ent", (float)id);
    }
    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
}

void Create_Game2()
{
    World *world        = World_Create_AllSys();
    world->doesSimulate = false;
    world->doesRender   = false;

    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD1;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
}
