#pragma once
#include "s_ai.h"

#include "estate.h"

extern const StateFunc aistate_func[AISTATE_COUNT];

bool Sol_Ai_SetState(World *world, int id, AiState nextState, u32 slot);
u32  Sol_Ai_FindTarget(World *world, int id);

void Sol_Ai_SetLastHit(World *world, int id, int source, float damage);
void Sol_Ai_TargetDied(World *world, int id, int target);

void Patrol_State_Update(World *world, int id, float dt);
void Patrol_State_Enter(World *world, int id);
void Patrol_State_Exit(World *world, int id);
bool Patrol_State_CanExit(World *world, int id, u32 next);
bool Patrol_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Idle_State_Update(World *world, int id, float dt);
void Idle_State_Enter(World *world, int id);
void Idle_State_Exit(World *world, int id);
bool Idle_State_CanExit(World *world, int id, u32 next);
bool Idle_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Aggro_State_Update(World *world, int id, float dt);
void Aggro_State_Enter(World *world, int id);
void Aggro_State_Exit(World *world, int id);
bool Aggro_State_CanExit(World *world, int id, u32 next);
bool Aggro_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);
