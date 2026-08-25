#pragma once
#include "sol/types.h"

typedef struct
{
    SolItem *items;
    int      cnt, cap;
    int      abilityBar[ABILITY_SLOTS];
} CompInventory;

typedef struct
{
    SolItem *items;
    int      cnt;
} InventoryDesc;

void           Sol_Inventory_Init(World *world);
CompInventory *Sol_Inventory_Add(World *world, int id, InventoryDesc desc);
CompInventory *Sol_Inventory_Get(World *world, int id);
bool           Sol_Inventory_Has(World *world, int id);
void           Sol_Inventory_Rem(World *world, int id);

int      Sol_Inventory_AddItem(World *world, int id, const SolItem item);
SolItem *Sol_Inventory_GetItemAtSlot(World *world, int id, int slot);