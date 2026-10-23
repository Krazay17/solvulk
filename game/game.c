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
    Create_Game3();
}

void Create_Menu()
{
    World *world        = World_Create();
    sol_user.menu_world = world->index;
    Sol_Sys_Add_Ui(world);

    { // QUIT BUTTON
        int id =
            Sol_Prefab_Button(world, (vec3s){1130.0f, 100.0f, 0}, "QUIT", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_Quit);
    }
    { // DEBUG BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){1130.0f, 150.0f, 0}, "Debug",
                                   INTERACTFLAG_DRAGGABLE | INTERACTFLAG_TOGGLEABLE, UILAYER_4, Hook_DebugToggle);
    }
    { // FULLSCREEN BUTTON
        int id = Sol_Prefab_Button(world, (vec3s){1130.0f, 200.0f, 0}, "FULLSCREEN",
                                   INTERACTFLAG_DRAGGABLE | INTERACTFLAG_TOGGLEABLE, UILAYER_4, Hook_Fullscreen);
    }

    GridMaker grid = {.cols = 4, .start = (vec2s){20.0f, 120.0f}, .spacing = (vec2s){120.0f, 40.0f}};
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Wizard", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SpawnWizard);
    { // WIZARDS BUTTON
        int id = Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Wizards", INTERACTFLAG_DRAGGABLE, UILAYER_4, NULL);
        ScHook *hook = Sol_Comp_Add(world, id, ScHook);
        hook->held   = Hook_SpawnWizard;
    }
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "World1", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SwitchWorld);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "World2", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SwitchWorld2);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "World3", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SwitchWorld3);
    Sol_Prefab_Slider(world, Sol_Grid_Next(&grid), "Volume", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SetVolume);
    Sol_Prefab_Slider(world, Sol_Grid_Next(&grid), "Timescale", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SetTimescale);
    Sol_Prefab_Slider(world, Sol_Grid_Next(&grid), "Fov", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SetPlayerFov);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Clone", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_Clone);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "AddItem", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_AddItem);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Save", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SaveUser);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Save Clear", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SaveClear);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Emitter", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SpawnEmitter);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Test", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_Test);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Respawn", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SpawnPlayer);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Dude", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SpawnDude);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "ClearEnts", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_ClearEnts);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Dudes", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SpawnDudes);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Reflect", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_TestReflect);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "TakeDamage", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_TakeDamage);
    {
        int team_slider =
            Sol_Prefab_Slider(world, Sol_Grid_Next(&grid), "Team", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_TeamChange);
        forc(world, team_slider, ScSlider) c->step = 0.5f;
    }
}

void Create_Hud()
{
    World *world       = World_Create();
    sol_user.hud_world = world->index;
    Sol_Sys_Add_Ui(world);
    Sol_Prefab_Crosshair(world);
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;

    Sol_Rules_Start(world, RULESKIND_CRYSTALPUSH);
    forc(world, 0, SlRules)c->on_win = OnRulesWin;
    // forc(world, 0, SlRules)
    // {
    //     c->as.crystal_push.wave_rate         = 25.0f;
    //     c->as.crystal_push._wave_accum       = c->as.crystal_push.wave_rate;
    //     c->as.crystal_push.wave_minion_count = 3;
    //     c->as.crystal_push.minion_rate       = 1.0f;

    //     c->as.crystal_push.crystal_a = Sol_Prefab_Crystal(world, (vec3s){0, 3, -90.0f}, 0);
    //     c->as.crystal_push.crystal_b = Sol_Prefab_Crystal(world, (vec3s){0, 3, 90.0f}, 1);

    //     c->kind   = RULESKIND_CRYSTALPUSH;
    //     c->on_win = OnRulesWin;

    // }
    { // Player
        int id            = Sol_Prefab_Dude(world, (vec3s){0, 6, -5}, 1.0f);
        sol_user.view_ent = id;
        Sol_Comp_Add(world, id, ScPlayer);
        ScUser *user = Sol_Comp_Add(world, id, ScUser);
        user->uid    = user_session.users[0].id;
        ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
        snprintf(meta->name, sizeof(meta->name), "Player");
        Sol_Debug_Add("Player Ent", (float)id);
    }
    // { // Ai Dude
    //     vec3s spawn_pos                           = {0, 6, 15};
    //     int id                                    = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
    //     Sol_Comp_Add(world, id, ScAi)->aggroRange = 100.0f;
    //     ScCombat *combat                          = Sol_Comp_Add(world, id, ScCombat);
    //     combat->respawnTime                       = 2.0f;
    //     combat->respawnPos                        = spawn_pos;
    // }
    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD11;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
}

void Create_Game2()
{
    World *world        = World_Create_AllSys();
    world->doesSimulate = false;
    world->doesRender   = false;
    // WAddStep(world)     = RandomizeSpawn;

    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD1;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
}

void Create_Game3()
{
    World *world        = World_Create_AllSys();
    world->doesSimulate = false;
    world->doesRender   = false;

    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
    for (int j = -2; j < 2; j++)
    {
        vec3s spawn_pos     = {-10, 4, j * 2};
        int id              = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
        ScCombat *combat    = Sol_Comp_Add(world, id, ScCombat);
        combat->respawnTime = 2.0f;
    }
    for (int j = -2; j < 2; j++)
    {
        vec3s spawn_pos     = {10, 4, j * 2};
        int id              = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
        ScCombat *combat    = Sol_Comp_Add(world, id, ScCombat);
        combat->respawnTime = 2.0f;
    }
    { // Evan dude
        vec3s spawn_pos                        = {0, 4, 5};
        int id                                 = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
        Sol_Comp_Get(world, id, ScModel)->kind = MODELKIND_EVANRIGGED;
        ScCombat *combat                       = Sol_Comp_Add(world, id, ScCombat);
        combat->respawnTime                    = 2.0f;
        ScMeta *meta                           = Sol_Comp_Add(world, id, ScMeta);
        strncpy(meta->name, "Evan", sizeof(meta->name));
    }
}
