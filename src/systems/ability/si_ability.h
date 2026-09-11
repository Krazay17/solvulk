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

typedef struct ScAbility ScAbility;
typedef struct ScCmd     ScCmd;

extern const AbilityStateFunc ABILITY_STATE_FUNC[ABILITY_STATE_COUNT];

void Ability_Script_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Script_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Script_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Script_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Script_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);

void Ability_Idle_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Idle_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Idle_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Idle_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Idle_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);

void Ability_Claw_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Claw_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Claw_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);
void Ability_Claw_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd);

void Ability_Dash_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Dash_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Dash_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Dash_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Dash_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);

void Ability_Shield_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Shield_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Shield_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Shield_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Shield_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);
void Ability_Shield_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd);

void Ability_Fireball_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);
void Ability_Fireball_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd);

void Ability_Pistol_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Pistol_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Pistol_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Pistol_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Pistol_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);

void Ability_Spinslash_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Spinslash_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Spinslash_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Spinslash_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Spinslash_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);

void Ability_Laser_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Laser_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Laser_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Laser_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Laser_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);
void Ability_Laser_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd);

void Ability_Whip_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt);
void Ability_Whip_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd);
void Ability_Whip_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd);
bool Ability_Whip_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next);
bool Ability_Whip_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot);