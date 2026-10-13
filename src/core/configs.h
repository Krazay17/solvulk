/*
 * File: configs.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-24
 *
 */
#pragma once
#include "sol/types.h"

extern const u32 item_ability_map[ITEMKIND_COUNT][3];
extern const u32 item_texture_map[ITEMKIND_COUNT];

extern const char *item_name[ITEMKIND_COUNT];
extern const char *ability_state_name[ABILITY_STATE_COUNT];
extern const char *move_state_name[MOVE_STATE_COUNT];
extern const WeaponKind ability_weapon_map[ITEMKIND_COUNT];

extern const vec4s rarity_color[ITEMRARITY_COUNT];