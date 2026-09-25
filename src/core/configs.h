/*
 * File: configs.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-24
 * 
*/
#pragma once
#include "sol/types.h"

extern const u32 abilitybar_slot_map[7];
extern const u32 abilityslot_state_map[ABILITYKIND_COUNT][3];
extern const u32 ability_texture_map[ABILITYKIND_COUNT];
extern AbilityConfig ability_base[ABILITYKIND_COUNT];

extern const char *ability_name[ABILITYKIND_COUNT];
extern const char *ability_state_name[ABILITY_STATE_COUNT];
extern const char *move_state_name[MOVE_STATE_COUNT];
