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

static inline void Hook_SpawnPlayer(World *w, int a, int b)
{
    World *game = Sol_User_GetGameWorld();

    int id = Sol_Prefab_Dude(game, (vec3s){0, 5, 0}, 1.0f);
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

static inline void Hook_Test(World *w, int a, int b)
{
    Sol_Comp_Add(w, a, ScPlayer);
}

static inline void Hook_SpawnWizard(World *w, int a, int b)
{
    World *world = Sol_User_GetGameWorld();
    int wizard   = Sol_Prefab_Wizard(world, (vec3s){0, 20.f, 0}, 1.0f);

    Sol_Comp_Add(world, wizard, ScHook)->release = Hook_Test;
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
    QuitApp(0);
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
    Sol_User_AddItem(&(SolItem){.ability.state = ABILITY_STATE_CLAW});
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
static inline void Hook_SpawnEmitter2(World *w, int a, int b)
{
    World *world = Sol_User_GetGameWorld();
    vec3s pos    = Xform_Get(world, sol_user.view_ent).pos;

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int id           = player_set->dense[i];
        ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
        combat->health -= 10.0f;
        sollog(combat->health);
    }
}