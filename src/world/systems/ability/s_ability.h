/*
 * File: s_ability.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#pragma once
#include "sol/types.h"
#include "estate.h"

extern const char         *ability_names[ABILITY_STATE_COUNT];
extern const StateFunc     ABILITY_STATE_FUNC[ABILITY_STATE_COUNT];
extern const AbilityConfig ability_rarity_base[ABILITY_STATE_COUNT][4];
extern const AbilityConfig ability_base[ABILITY_STATE_COUNT];

void Script_State_Update(World *world, int id, float dt);
void Script_State_Enter(World *world, int id);
void Script_State_Exit(World *world, int id);
bool Script_State_CanExit(World *world, int id, u32 nextState);
bool Script_State_CanEnter(World *world, int id, u32 lastState, u32 next, int slot);

void IdleAbility_State_Update(World *world, int id, float dt);
void IdleAbility_State_Enter(World *world, int id);
void IdleAbility_State_Exit(World *world, int id);
bool IdleAbility_State_CanExit(World *world, int id, u32 next);
bool IdleAbility_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Claw_State_Update(World *world, int id, float dt);
void Claw_State_Enter(World *world, int id);
void Claw_State_Exit(World *world, int id);
bool Claw_State_CanExit(World *world, int id, u32 next);
bool Claw_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void ADash_State_Update(World *world, int id, float dt);
void ADash_State_Enter(World *world, int id);
void ADash_State_Exit(World *world, int id);
bool ADash_State_CanExit(World *world, int id, u32 next);
bool ADash_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Shield_State_Update(World *world, int id, float dt);
void Shield_State_Enter(World *world, int id);
void Shield_State_Exit(World *world, int id);
bool Shield_State_CanExit(World *world, int id, u32 next);
bool Shield_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);
void Shield_State_Draw(World *world, int id);

void Fireball_State_Update(World *world, int id, float dt);
void Fireball_State_Enter(World *world, int id);
void Fireball_State_Exit(World *world, int id);
bool Fireball_State_CanExit(World *world, int id, u32 next);
bool Fireball_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);
void Fireball_State_Draw(World *world, int id);

void Pistol_State_Update(World *world, int id, float dt);
void Pistol_State_Enter(World *world, int id);
void Pistol_State_Exit(World *world, int id);
bool Pistol_State_CanExit(World *world, int id, u32 next);
bool Pistol_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Spinslash_State_Update(World *world, int id, float dt);
void Spinslash_State_Enter(World *world, int id);
void Spinslash_State_Exit(World *world, int id);
bool Spinslash_State_CanExit(World *world, int id, u32 next);
bool Spinslash_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Laser_State_Update(World *world, int id, float dt);
void Laser_State_Enter(World *world, int id);
void Laser_State_Exit(World *world, int id);
bool Laser_State_CanExit(World *world, int id, u32 next);
bool Laser_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);
void Laser_State_Draw(World *world, int id);

void Whip_State_Update(World *world, int id, float dt);
void Whip_State_Enter(World *world, int id);
void Whip_State_Exit(World *world, int id);
bool Whip_State_CanExit(World *world, int id, u32 next);
bool Whip_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);