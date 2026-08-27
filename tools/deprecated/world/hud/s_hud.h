/*
 * File: s_hud.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-25
 *
 */

#pragma once
#include "sol/types.h"

#define MAX_ITEMS 512
#define ABILITYBAR_WIDTH 442.0f

typedef struct
{
    int idx;
} CompHudItem;

typedef struct CompAbilitySlot
{
    int  slot;
    bool onCooldown;
} CompHudSlot;

void Sol_Hud_Init(World *world);

CompHudItem *Sol_Hud_AddItem(World *world, int id);
CompHudItem *Sol_Hud_GetItem(World *world, int id);
CompHudSlot *Sol_Hud_AddSlot(World *world, int id);
CompHudSlot *Sol_Hud_GetSlot(World *world, int id);
void         Sol_Hud_RemItem(World *world, int id);
void         Sol_Hud_RemSlot(World *world, int id);