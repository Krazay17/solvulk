#pragma once
#include "base.h"

typedef struct World World;

typedef enum
{
    ITEMKIND_ABILITY_CARD,
    ITEMKIND_ABILITY_SLOT,
    ITEMKIND_COUNT,
} ItemKind;
typedef struct CompItem
{
    ItemKind kind;
    u32      ability;
    u32      slot;
    u8       rarity;
    bool     onCooldown;

    float bonusDamage;
    u32   bonusBuffs;
    u32   bonusEffects;
} CompItem;

void      Sol_Item_Init(World *world);
CompItem *Sol_Item_Add(World *world, int id, ItemKind kind);
void      Sol_Item_AddAbility(World *world, int id, u32 ability);
void      Sol_Item_AddAbilitySlot(World *world, int id, int slot);
void      Sol_Item_SetRarity(World *world, int id, u32 rarity);
void      Sol_Item_Drop(World *world, int id);
