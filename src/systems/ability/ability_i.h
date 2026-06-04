#pragma once
#include "sol/types.h"

#include "ability.h"
#include "estate.h"

extern const StateFunc ability_state_func[];

void Ability_Scripts_Init(void);
void Ability_BoostToDir(World *world, int id, vec3s dir, float speed);

void Script_State_Update(World *world, int id, float dt);
void Script_State_Enter(World *world, int id);
void Script_State_Exit(World *world, int id);
bool Script_State_CanExit(World *world, int id, u32 nextState);
bool Script_State_CanEnter(World *world, int id, u32 lastState, u32 next, u32 slot);

void IdleAbility_State_Update(World *world, int id, float dt);
void IdleAbility_State_Enter(World *world, int id);
void IdleAbility_State_Exit(World *world, int id);
bool IdleAbility_State_CanExit(World *world, int id, u32 next);
bool IdleAbility_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);

void Claw_State_Update(World *world, int id, float dt);
void Claw_State_Enter(World *world, int id);
void Claw_State_Exit(World *world, int id);
bool Claw_State_CanExit(World *world, int id, u32 next);
bool Claw_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);

void ADash_State_Update(World *world, int id, float dt);
void ADash_State_Enter(World *world, int id);
void ADash_State_Exit(World *world, int id);
bool ADash_State_CanExit(World *world, int id, u32 next);
bool ADash_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);

void Shield_State_Update(World *world, int id, float dt);
void Shield_State_Enter(World *world, int id);
void Shield_State_Exit(World *world, int id);
bool Shield_State_CanExit(World *world, int id, u32 next);
bool Shield_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);

void Fireball_State_Update(World *world, int id, float dt);
void Fireball_State_Enter(World *world, int id);
void Fireball_State_Exit(World *world, int id);
bool Fireball_State_CanExit(World *world, int id, u32 next);
bool Fireball_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);

void Pistol_State_Update(World *world, int id, float dt);
void Pistol_State_Enter(World *world, int id);
void Pistol_State_Exit(World *world, int id);
bool Pistol_State_CanExit(World *world, int id, u32 next);
bool Pistol_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);

void Spinslash_State_Update(World *world, int id, float dt);
void Spinslash_State_Enter(World *world, int id);
void Spinslash_State_Exit(World *world, int id);
bool Spinslash_State_CanExit(World *world, int id, u32 next);
bool Spinslash_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot);
