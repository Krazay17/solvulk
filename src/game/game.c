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

    int counter   = 0;
    vec2s start   = {20.0f, 120.0f};
    vec2s spacing = {120.0f, 40.0f};
    { // WIZARD BUTTON
        int id = Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Wizard",
                                   INTERACT_DRAGGABLE, UILAYER_4, Hook_SpawnWizard);
    }
    { // WIZARDS BUTTON
        int id       = Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Wizards",
                                         INTERACT_DRAGGABLE, UILAYER_4, NULL);
        ScHook *hook = Sol_Comp_Add(world, id, ScHook);
        hook->held   = Hook_SpawnWizard;
    }
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "World1", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SwitchWorld);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "World2", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SwitchWorld2);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "World3", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SwitchWorld3);
    Sol_Prefab_Slider(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Volume", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SetVolume);
    Sol_Prefab_Slider(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Timescale", INTERACT_DRAGGABLE,
                      UILAYER_4, Hook_SetTimescale);
    Sol_Prefab_Slider(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Fov", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SetPlayerFov);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Clone", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_Clone);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "AddItem", INTERACT_DRAGGABLE,
                      UILAYER_4, Hook_AddItem);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Save", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SaveUser);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Save Clear", INTERACT_DRAGGABLE,
                      UILAYER_4, Hook_SaveClear);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Emitter", INTERACT_DRAGGABLE,
                      UILAYER_4, Hook_SpawnEmitter);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Test", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_Test);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Respawn", INTERACT_DRAGGABLE,
                      UILAYER_4, Hook_SpawnPlayer);
    int dudebutton = Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Dude",
                                       INTERACT_DRAGGABLE, UILAYER_4, Hook_SpawnDude);
    // Sol_Comp_Get(world, dudebutton, ScHook)->held = Hook_SpawnDude;
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "ClearEnts", INTERACT_DRAGGABLE,
                      UILAYER_4, Hook_ClearEnts);
    Sol_Prefab_Button(world, Sol_GridMakerInc(&counter, 4, 12, start, spacing), "Dudes", INTERACT_DRAGGABLE, UILAYER_4,
                      Hook_SpawnDudes);
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
    Sol_Sys_Add(world, WORLDSYS_REF);

    Sol_Prefab_Crosshair(world);
}

void RandomizeSpawn(World *w, double dt)
{
    SlEvent *events = Sol_Comp_Get(w, 0, SlEvent);
    for (int i = 0; i < solb_count(events->events); i++)
    {
        SolEvent *event = &events->events[i];
        if (event->kind != EVENTKIND_DEATH)
            continue;
        int id           = event->entB;
        ScCombat *combat = Sol_Comp_Get(w, id, ScCombat);
        if (combat)
        {
            vec3s pos  = {0};
            bool found = false;
            float start_y = 200.0f;
            // Cap attempts to prevent frame freezes
            const int MAX_TRIES = 20;
            for (int attempt = 0; attempt < MAX_TRIES && !found; attempt++)
            {
                pos.x = Sol_Math_RandRange2(-300.0f, 300.0f);
                pos.z = Sol_Math_RandRange2(-300.0f, 300.0f);
                pos.y = start_y;

                SolRayResult result;
                // Extended ray distance to reach ground below Y = 0
                SolRay ray = {.start = pos, .dir = WORLD_DOWN, .dist = start_y + 10.0f};

                if (Sol_Raycast1D(w, ray, &result, 0.5f) && result.t > 0.0f)
                {
                    // Snap Y to the actual ground hit position + slight offset
                    pos.y = pos.y - result.t + 0.5f;
                    found = true;
                }
            }

            // Fallback location if all random attempts miss ground
            if (!found)
            {
                pos = (vec3s){0.0f, 5.0f, 0.0f}; // Default safe spawn point
            }

            combat->respawnPos = pos;
        }
    }
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;
    // WAddStep(world)     = RandomizeSpawn;

    { // Player
        int id            = Sol_Prefab_Dude(world, (vec3s){0, 6, -5}, 1.0f);
        sol_user.view_ent = id;
        Sol_Comp_Add(world, id, ScPlayer);
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
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
    // for (int j = -2; j < 2; j++)
    // {
    //     vec3s spawn_pos     = {-10, 4, j * 2};
    //     int id              = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
    //     ScCombat *combat    = Sol_Comp_Add(world, id, ScCombat);
    //     combat->respawnTime = 2.0f;
    //     combat->respawnPos  = spawn_pos;
    // }
    // for (int j = -2; j < 2; j++)
    // {
    //     vec3s spawn_pos     = {10, 4, j * 2};
    //     int id              = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
    //     ScCombat *combat    = Sol_Comp_Add(world, id, ScCombat);
    //     combat->respawnTime = 2.0f;
    //     combat->respawnPos  = spawn_pos;
    // }
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
}
