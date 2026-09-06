/*
 * File: hooks.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "game.h"

void Hook_Test(World *world, double dt, int id, void *data)
{
    ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
    if (body)
        body->vel.y += 50.0f;
}

void Hook_Quit(World *w, double dt, int id, void *data)
{
    QuitApp(0);
}

void Hook_Fullscreen(World *w, double dt, int id, void *data)
{
    W_Set_Fullscreen(Sol_Comp_Get(w, id, ScInteract)->state & INTERACT_TOGGLED);
}

void Hook_Healthbar(World *w, double dt, int id, void *data)
{
    World   *game_world = Sol_User_GetGameWorld();
    ScView2 *view2      = Sol_Comp_Get(w, id, ScView2);
    if (!game_world || !view2)
        return;

    float totalHealth    = 0.0f;
    float totalMaxHealth = 0.0f;

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game_world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int id = player_set->dense[i];

        ScCombat *combat = Sol_Comp_Get(game_world, id, ScCombat);
        if (!combat)
            continue;
        totalHealth += combat->health;
        totalMaxHealth += combat->maxHealth;
    }
    if (totalMaxHealth > 0.0f)
        view2->views[0].targetFill = clamp(totalHealth / totalMaxHealth, 0.0f, 1.0f);
    else
        view2->views[0].targetFill = 0.0f;
}
