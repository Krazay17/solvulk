/*
 * File: game.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#pragma once

#include "sol/sol.h"
#include "prefabs.h"

void Create_Sol_Game();

void QuitApp(int flags);
void W_Set_Ontop(int flags);
void W_Set_Fullscreen(int flags);

void Create_Menu();
void Create_Hud();
void Create_Game();
void Create_Game2();

extern const ScCamera player_camera;

static inline void Hook_SpawnPlayer(World *w, int a, int b)
{
    World *game = Sol_User_GetGameWorld();

    // SparseSet_ScPlayer *player_set = Sol_Comp_Set(game, ScPlayer);
    // player_set->cnt                = 0;
    int id                         = Sol_Prefab_Dude(game, (vec3s){0, 5, 0}, 1.0f);
    Sol_Comp_Add(game, id, ScPlayer);
    sol_user.view_ent = id;
    ScMeta *meta      = Sol_Comp_Add(game, id, ScMeta);
    snprintf(meta->name, sizeof(meta->name), "Player");
    Sol_Debug_Add("Player Ent", (float)id);
}

static inline void Hook_SwitchWorld(World *w, int a, int b)
{
    Sol_User_EnterGameWorld(2, true, (vec3s){0, 5, 0});
}

static inline void Hook_SwitchWorld2(World *w, int a, int b)
{
    Sol_User_EnterGameWorld(3, true, (vec3s){0, 5, 0});
}

static inline void Hook_Possess(World *w, int a, int b)
{
    Sol_Comp_Rem(w, sol_user.view_ent, ScPlayer);

    *Sol_Comp_Add(w, a, ScCamera) = player_camera;
    sol_user.view_ent             = a;
    Sol_Comp_Add(w, a, ScPlayer);
    Sol_Comp_Rem(w, a, ScAi);

    Sol_Debug_Add("Player Ent", (float)a);
}

static inline void Hook_SpawnWizard(World *w, int a, int b)
{
    World *world = Sol_User_GetGameWorld();
    float fdt    = world->fdt;

    int wizard = Sol_Prefab_Wizard(world, (vec3s){sinf(fdt), 20.f, cosf(fdt)}, 1.0f);

    Sol_Comp_Add(world, wizard, ScHook)->release = Hook_Possess;
}
static inline void Hook_SpawnDude(World *w, int a, int b)
{
    World *world = Sol_User_GetGameWorld();
    float fdt    = world->fdt;

    vec3s spawn_pos                           = {sinf(fdt), 10.f, cosf(fdt)};
    int id                                    = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
    Sol_Comp_Add(world, id, ScAi)->aggroRange = 20.0f;
    ScCombat *combat                          = Sol_Comp_Add(world, id, ScCombat);
    combat->respawnTime                       = 2.0f;
    combat->respawnPos                        = spawn_pos;
}
static inline void Hook_DebugToggle(World *w, int a, int b)
{
    solState.debug = (Sol_Comp_Get(w, a, ScInteract)->state & INTERACT_TOGGLED) != 0;
}
static inline void Hook_CrystalDrain(World *w, int a, int b)
{
}
static inline void Hook_Quit(World *w, int a, int b)
{
    solState.destroy_qued = true;
}
static inline void Hook_Fullscreen(World *w, int a, int b)
{
    W_Set_Fullscreen(Sol_Comp_Get(w, a, ScInteract)->state & INTERACT_TOGGLED);
}
static inline void Hook_Healthbar(World *w, int a, int b)
{
    World *game_world = Sol_User_GetGameWorld();
    ScView2 *view2    = Sol_Comp_Get(w, a, ScView2);
    if (!game_world || !view2)
        return;

    float totalMaxHealth = 0.0f;
    float totalHealth    = 0.0f;

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);

    for (int i = 0; i < player_set->cnt; i++)
    {
        int id           = player_set->dense[i];
        ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
        if (!combat)
            continue;
        totalHealth += combat->health;
        totalMaxHealth += combat->healthMax;
    }

    if (totalMaxHealth > 0.0f)
    {
        view2->views[2].targetFill = clamp(totalHealth / totalMaxHealth, 0.0f, 1.0f);
        view2->views[3].targetFill = clamp(totalHealth / totalMaxHealth, 0.0f, 1.0f);
    }
    else
    {
        view2->views[2].targetFill = 0.0f;
        view2->views[3].targetFill = 0.0f;
    }
}

static inline void Hook_SetVolume(World *w, int a, int b)
{
}

static inline void Hook_SunAngle(World *w, int a, int b)
{
}

static inline void Hook_Clone(World *w, int a, int b)
{
    World *world = Sol_User_GetGameWorld();
    vec3s pos    = Xform_Get(world, sol_user.view_ent).pos;
    int id       = Sol_Prefab_Dude(world, pos, 1.0f);
    Sol_Comp_Add(world, id, ScPlayer);
}

static inline void Hook_AddItem(World *w, int a, int b)
{
    Sol_User_AddItem(&(SolItem){.kind = ABILITY_STATE_CLAW, .effects = EFFECTMASK_KNOCKUP});
    Sol_User_AddItem(&(SolItem){.kind = ABILITY_STATE_FIREBALL, .effects = EFFECTMASK_KNOCKUP});
}

static inline void Hook_SaveUser(World *w, int a, int b)
{
    Sol_User_SaveUserSettings();
}
static inline void Hook_SaveClear(World *w, int a, int b)
{
    Sol_User_ClearUserSettings();
}
static inline void Hook_SpawnEmitter(World *w, int a, int b)
{
    World *world = Sol_User_GetGameWorld();
    vec3s pos    = Xform_Get(world, sol_user.view_ent).pos;

    Sol_Prefab_PlasmaOrb(world, pos);
}
static inline void Hook_SetPlayerFov(World *w, int a, int b)
{
    World *game = Sol_User_GetGameWorld();
    int id      = sol_user.view_ent;

    ScSlider *slider = Sol_Comp_Get(w, a, ScSlider);
    ScCamera *camera = Sol_Comp_Get(game, id, ScCamera);
    if (camera && slider)
        camera->fov = Sol_Math_MapRange(60.0f, 120.f, 0, 1.0f, slider->value);
}
static inline void Hook_ClearEnts(World *w, int a, int b)
{
    World *game = Sol_User_GetGameWorld();
    int i       = 1;
    while (i < game->entCount)
    {
        int id = game->dense[i];
        if (Sol_Comp_Has(game, id, ScPlayer) || Sol_Comp_Has(game, id, ScStage))
        {
            i++;
        }
        else
        {
            Sol_Destroy_Ent(game, id);
        }
    }
}
static inline void Hook_SetTimescale(World *w, int a, int b)
{
    World *game      = Sol_User_GetGameWorld();
    ScSlider *slider = Sol_Comp_Get(w, a, ScSlider);
    if (game && slider)
    {
        game->timescale = Sol_Math_MapRange(0.0f, 5.0f, 0, 1.0f, slider->value);
    }
}

static inline void Hook_Test(World *w, int a, int b)
{
    World *game = Sol_User_GetGameWorld();
    int id      = sol_user.view_ent;
    vec3s pos   = Xform_Get(game, sol_user.view_ent).pos;
    Sol_Buff_AddMask(game, id, 1, 0, 1.0f);
}
