#pragma once
#include "base.h"

typedef struct Item
{
    u32  ability;
    u32  slot;
    u8   rarity;
    bool onCooldown;

    float bonusDamage;
    u32   bonusBuffs;
    u32   bonusEffects;
} Item;

typedef struct CompItem
{
    Item item;
} CompItem;

typedef struct CompAbilitySlot
{
    int slot;
    bool onCooldown;
} CompAbilitySlot;

typedef struct CompInventory
{
    Item *items;
    int   cnt, cap;
} CompInventory;

void Sol_Item_Init(World *world);
void Sol_Item_AddAbility(World *world, int id, u32 ability);
void Sol_Item_AddAbilitySlot(World *world, int id, int slot);
void Sol_Item_SetRarity(World *world, int id, u32 rarity);
void Sol_Item_Drop(World *world, int id);

void Sol_Inventory_AddItem(World *world, int id, Item item);
