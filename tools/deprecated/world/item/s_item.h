#pragma once
#include "sol/types.h"

extern const AbilityConfig item_rarity_base[ABILITY_STATE_COUNT][4];

void      Sol_Item_Init(World *world);
// CompItem *Sol_Item_Add(World *world, int id);
// CompItem *Sol_Item_Get(World *world, int id);
bool      Sol_Item_Has(World *world, int id);
void      Sol_Item_Rem(World *world, int id);

void Sol_Item_SetRarity(World *world, int id, u32 rarity);

SolItem *Sol_Item_GetItemAtSlot(World *world, int id, int slot);
void     Sol_Item_Drop(World *world, int id);
