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

void Hook_Test(World *world, double dt, int id, void *data);
void Hook_Quit(World *w, double dt, int id, void *data);
void Hook_Fullscreen(World *w, double dt, int id, void *data);
void Hook_Healthbar(World *w, double dt, int id, void *data);

static inline void Hook_SwitchWorld(World *w, double dt, int id, void *data)
{
    Sol_User_EnterGameWorld(2, true, (vec3s){0,5,0});
}
static inline void Hook_SwitchWorld2(World *w, double dt, int id, void *data)
{
    Sol_User_EnterGameWorld(3, true, (vec3s){0,5,0});
}

static inline void Hook_SpawnWizard(World *w, double dt, int id, void *data)
{
    Sol_Prefab_Wizard(Sol_User_GetGameWorld(), (vec3s){0, 20.f, 0}, 1.0f);
}