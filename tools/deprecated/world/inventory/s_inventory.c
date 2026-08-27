#include "s_inventory.h"
#include "world.h"

typedef struct
{
    int            cnt, cap;
    int           *sparse, *dense;
    CompInventory *inventories;
} WorldInventories;

void Sol_Inventory_Init(World *world)
{
    WorldInventories *wc                         = malloc(sizeof(WorldInventories));
    world->dense_components[WORLD_SYS_INVENTORY] = wc;
    wc->cap                                      = 16;
    wc->cnt                                      = 0;
    wc->sparse                                   = malloc(sizeof(int) * MAX_ENTS);
    wc->dense                                    = malloc(sizeof(int) * wc->cap);
    wc->inventories                              = malloc(sizeof(CompInventory) * wc->cap);
    memset(wc->sparse, -1, sizeof(int) * MAX_ENTS);
}

CompInventory *Sol_Inventory_Add(World *world, int id, InventoryDesc desc)
{
    if (id < 0 || id >= MAX_ENTS)
        return NULL;
    WorldInventories *wc = world->dense_components[WORLD_SYS_INVENTORY];
    if (wc->sparse[id] != -1)
        return &wc->inventories[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense       = realloc(wc->dense, sizeof(int) * wc->cap);
        wc->inventories = realloc(wc->inventories, sizeof(CompInventory) * wc->cap);
    }
    int idx        = wc->cnt++;
    wc->sparse[id] = idx;
    wc->dense[idx] = id;

    CompInventory *inventory = &wc->inventories[idx];
    inventory->cnt           = desc.cnt;
    inventory->cap           = desc.cnt > 0 ? desc.cnt : 4; // Ensure non-zero initial capacity
    inventory->items         = malloc(sizeof(SolItem) * inventory->cap);
    memset(inventory->abilityBar, -1, sizeof(inventory->abilityBar));
    if (desc.items && desc.cnt > 0)
        memcpy(inventory->items, desc.items, sizeof(SolItem) * desc.cnt);

    return inventory;
}

CompInventory *Sol_Inventory_Get(World *world, int id)
{
    if (id < 0 || id >= MAX_ENTS)
        return NULL;

    WorldInventories *wc = world->dense_components[WORLD_SYS_INVENTORY];
    if (wc->sparse[id] != -1)
        return &wc->inventories[wc->sparse[id]];
    return NULL;
}

bool Sol_Inventory_Has(World *world, int id)
{
    if (id < 0 || id >= MAX_ENTS)
        return false;

    WorldInventories *wc = world->dense_components[WORLD_SYS_INVENTORY];
    return wc->sparse[id] != -1;
}

void Sol_Inventory_Rem(World *world, int id)
{
    if (id < 0 || id >= MAX_ENTS)
        return;

    WorldInventories *wc  = world->dense_components[WORLD_SYS_INVENTORY];
    int               idx = wc->sparse[id];
    if (idx != -1)
    {
        free(wc->inventories[idx].items);
        int last_idx = wc->cnt - 1;
        int last_id  = wc->dense[last_idx];

        if (idx != last_idx)
        {
            wc->dense[idx]       = last_id;
            wc->sparse[last_id]  = idx;
            wc->inventories[idx] = wc->inventories[last_idx];
        }

        wc->sparse[id] = -1;
        wc->cnt--;
    }
}

int Sol_Inventory_AddItem(World *world, int id, const SolItem item)
{
    if (id < 0 || id >= MAX_ENTS)
        return -1;

    CompInventory *inventory = Sol_Inventory_Get(world, id);
    if (!inventory)
        return -1;
    if (inventory->cnt >= inventory->cap)
    {
        inventory->cap   = (inventory->cap == 0) ? 4 : inventory->cap * 2;
        inventory->items = realloc(inventory->items, sizeof(SolItem) * inventory->cap);
    }
    int idx               = inventory->cnt++;
    inventory->items[idx] = item;

    return idx;
}

SolItem *Sol_Inventory_GetItemAtSlot(World *world, int id, int slot)
{
    if (id < 0 || id >= MAX_ENTS)
        return NULL;

    CompInventory *inv = Sol_Inventory_Get(world, id);
    if (!inv)
        return NULL;
    SolItem *item = &inv->items[inv->abilityBar[slot]];
    if (!item)
        return NULL;

    return item;
}

void Sol_Inventory_SetAbilitySlot(World *world, int id, int idx, int slot)
{
    Sol_Inventory_Get(world, id)->abilityBar[idx] = slot;
}
