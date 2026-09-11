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

static inline void Hook_SwitchWorld(World *w, int id, int interactor, double dt, void *data)
{
    Sol_User_EnterGameWorld(2, true, (vec3s){0, 5, 0});
}

static inline void Hook_SwitchWorld2(World *w, int id, int interactor, double dt, void *data)
{
    Sol_User_EnterGameWorld(3, true, (vec3s){0, 5, 0});
}

static inline void Hook_Test(World *w, int id, int interactor, double dt, void *data)
{
    ScBody3 *body = Sol_Comp_Get(w, id, ScBody3);
    if (body)
        body->vel.y += 50.0f;
}

static inline void Hook_SpawnWizard(World *w, int id, int interactor, double dt, void *data)
{
    World *world = Sol_User_GetGameWorld();
    int wizard   = Sol_Prefab_Wizard(world, (vec3s){0, 20.f, 0}, 1.0f);

    Sol_Comp_Add(world, wizard, ScHook)->release = Hook_Test;
}

static inline void Hook_DebugToggle(World *w, int id, int interactor, double dt, void *data)
{
    solState.debug = (Sol_Comp_Get(w, id, ScInteract)->state & INTERACT_TOGGLED) != 0;
}

static inline void Hook_CrystalDrain(World *w, int id, int interactor, double dt, void *data)
{
}

static inline void Hook_Quit(World *w, int id, int interactor, double dt, void *data)
{
    QuitApp(0);
}

static inline void Hook_Fullscreen(World *w, int id, int interactor, double dt, void *data)
{
    W_Set_Fullscreen(Sol_Comp_Get(w, id, ScInteract)->state & INTERACT_TOGGLED);
}

static inline void Hook_Healthbar(World *w, int id, int interactor, double dt, void *data)
{
    World *game_world = Sol_User_GetGameWorld();
    ScView2 *view2    = Sol_Comp_Get(w, id, ScView2);
    if (!game_world || !view2)
        return;

    float totalMaxHealth = 0.0f;
    float totalHealth    = 0.0f;

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int id = player_set->dense[i];

        ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
        if (!combat)
            continue;
        totalHealth += combat->health;
        totalMaxHealth += combat->healthMax;
    }
    if (totalMaxHealth > 0.0f)
        view2->views[0].targetFill = clamp(totalHealth / totalMaxHealth, 0.0f, 1.0f);
    else
        view2->views[0].targetFill = 0.0f;
}

static inline void Hook_SetVolume(World *w, int id, int interactor, double dt, void *data)
{
}

static inline void Hook_SunAngle(World *w, int id, int interactor, double dt, void *data)
{
}

static inline void Hook_Clone(World *w, int a, int b, double dt, void *data)
{
    World *world = Sol_User_GetGameWorld();
    vec3s pos    = Xform_Get(world, sol_user.view_ent).pos;
    int id       = Sol_Prefab_Dude(world, pos, 1.0f);
    Sol_Comp_Add(world, id, ScPlayer);
}