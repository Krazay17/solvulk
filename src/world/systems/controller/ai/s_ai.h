/*
 * File: s_ai.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-08
 *
 */
#pragma once
#include "sol/types.h"
#include "estate.h"

extern const AiStateFuncs ai_funcs_base[AISTATE_COUNT];
extern const AiStateFuncs ai_funcs[AIKIND_COUNT][AISTATE_COUNT];

static inline AiStateFuncs Ai_Get_Funcs(AiKind kind, AiState state)
{
    AiStateFuncs f    = ai_funcs[kind][state];
    AiStateFuncs base = ai_funcs_base[state];

    if (!f.update)
        f.update = base.update;
    if (!f.enter)
        f.enter = base.enter;
    if (!f.exit)
        f.exit = base.exit;
    if (!f.canEnter)
        f.canEnter = base.canEnter;
    if (!f.canExit)
        f.canExit = base.canExit;

    return f;
}

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team);

void Ai_Idle_Update(World *world, int id, ScAi *ai, float dt);
void Ai_Idle_Enter(World *world, int id, ScAi *ai);
void Ai_Idle_Exit(World *world, int id, ScAi *ai);
bool Ai_Idle_CanExit(World *world, int id, ScAi *ai, u32 next);
bool Ai_Idle_CanEnter(World *world, int id, ScAi *ai, u32 last);

void Ai_Patrol_Update(World *world, int id, ScAi *ai, float dt);
void Ai_Patrol_Enter(World *world, int id, ScAi *ai);
void Ai_Patrol_Exit(World *world, int id, ScAi *ai);
bool Ai_Patrol_CanExit(World *world, int id, ScAi *ai, u32 next);
bool Ai_Patrol_CanEnter(World *world, int id, ScAi *ai, u32 last);

void Ai_Aggro_Update(World *world, int id, ScAi *ai, float dt);
void Ai_Aggro_Enter(World *world, int id, ScAi *ai);
void Ai_Aggro_Exit(World *world, int id, ScAi *ai);
bool Ai_Aggro_CanExit(World *world, int id, ScAi *ai, u32 next);
bool Ai_Aggro_CanEnter(World *world, int id, ScAi *ai, u32 last);

void Ai_Wizard_Aggro_Update(World *world, int id, ScAi *ai, float dt);
