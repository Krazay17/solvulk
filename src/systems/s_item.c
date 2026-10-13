/*
 * File: s_item.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-30
 *
 */
#include "world.h"
#include "sol_user.h"
#include "sol_core.h"

const vec4s rarity_color[ITEMRARITY_COUNT] = {{0.2f, 0.2f, 0.2f, 1.0f},
                                              {0.0f, 1.0f, 0.0f, 1.0f},
                                              {0.0f, 0.0f, 1.0f, 1.0f},
                                              {0.0f, 1.0f, 1.0f, 1.0f},
                                              {1.0f, 0.3f, 0.0f, 1.0f}};

enum
{
    COOLDOWN,
    DAMAGE,
    MAXPOWER,
    SPEED,
    PARAMCOUNT,
};

float item_rarity_scale[PARAMCOUNT] = {
    [COOLDOWN] = 0.1f,
    [DAMAGE]   = 0.1f,
    [MAXPOWER] = 0.1f,
    [SPEED]    = 0.1f,
};

bool g_items_active[256] = {0};
void Item_User_Update(World *world, SolItem *items, int count)
{    
    SparseSet_ScRef *ref_set = Sol_Comp_Set(world, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int id     = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_LOOSEITEM)
            continue;
        if (ref->index < 0 || ref->index >= count)
        {
            Sol_Destroy_Ent(world, id);
            continue;
        }
        g_items_active[ref->index] = true;
    }
    for (int i = 0; i < count; i++)
    {
        if (!g_items_active[i])
        {
            SolItem *item = &items[i];
            float x       = 0.0f + (i % 5) * 64.0f;
            float y       = 0.0f + (i / 5) * 64.0f;
            int card_id   = Sol_Prefab_AbilityCard(world, (vec3s){x, y, 0}, item);

            *Sol_Comp_Add(world, card_id, ScRef) = (ScRef){.kind = REFKIND_LOOSEITEM, .index = i};
        }
    }
}

void Inventory_Update(World *world, double dt)
{
    // SparseSet_ScInventory *inv_set = Sol_Comp_Set(world, ScInventory);
    // for(int i=0;i<inv_set->cnt;i++)
    // {
    //     int id = inv_set->dense[i];
    //     ScInventory *inv = &inv_set->data[i];
    //     forc(world, id, ScAbility)
    //     {
    //         for(int j = 5;j<ABILITY_SLOTS;j++)
    //         {
    //             Sol_Ability_Equip(world, id, j, &inv->equipped_items[j]);
    //         }
    //     }
    // }
}
void Item_Update(World *world, double dt)
{
    Inventory_Update(world, dt);
}

AbilityConfig Sol_Item_GetMods(SolItem item)
{
    AbilityConfig conf = {0};
    if (item.item_kind > 0 && item.item_kind < ITEMKIND_COUNT)
    {
        conf.cooldown = (item_rarity_scale[COOLDOWN] * (float)item.rarity);
        conf.maxpower = (item_rarity_scale[MAXPOWER] * (float)item.rarity);
        conf.speed    = (item_rarity_scale[SPEED] * (float)item.rarity);

        conf.damage.amount     = (item_rarity_scale[DAMAGE] * (float)item.rarity);
        conf.damage.buffMask   = item.buffMask;
        conf.damage.effectMask = item.effectMask;
    }
    return conf;
}

AbilityConfig Sol_Item_ApplyMods(AbilityConfig conf, SolItem item)
{
    if (item.item_kind > 0 && item.item_kind < ITEMKIND_COUNT)
    {
        conf.cooldown *= 1.0f - (item_rarity_scale[COOLDOWN] * (float)item.rarity);
        conf.maxpower *= 1.0f + (item_rarity_scale[MAXPOWER] * (float)item.rarity);
        conf.speed *= 1.0f + (item_rarity_scale[SPEED] * (float)item.rarity);
        conf.damage.amount *= 1.0f + (item_rarity_scale[DAMAGE] * (float)item.rarity);
        conf.damage.buffMask |= item.buffMask;
        conf.damage.effectMask |= item.effectMask;
    }
    return conf;
}

SolItem *Sol_Item_Resolve(World *world, ScRef *ref)
{
    SolItem *item    = NULL;
    World *ref_world = Sol_GetWorldByIdx(ref->ent_world);
    if (!ref_world)
        return item;
        
    switch (ref->kind)
    {
    case REFKIND_INVENTORYITEM:
        forc(ref_world, ref->ent_id, ScInventory) item = &c->items[ref->index];
        break;
    case REFKIND_LOOSEITEM:
        item = &user_data.loose_items[ref->index];
        break;
    }

    return item;
}

void Sol_Inv_Equip(World *world, int id, int slot, SolItem item)
{    
    // forc(world, id, ScUser)
    // {
    //     user_data.inventory.equipped_items[slot] = item;
    //     return;
    // }
    forc(world, id, ScInventory)
    {
        c->equipped_items[slot] = item;
    }
    Sol_Equip_Item(world, id, item.item_kind, slot);
}
SolItem Sol_Inv_Remove(World *world, int id, u32 idx)
{
    SolItem item = {0};
    // forc(world, id, ScUser)
    // {
    //     item = user_data.inventory.items[idx];

    //     user_data.inventory.items[idx] = user_data.inventory.items[user_data.inventory.item_count - 1];
    //     user_data.inventory.item_count--;
    //     return item;
    // }
    forc(world, id, ScInventory)
    {
        item          = c->items[idx];
        c->items[idx] = c->items[c->item_count - 1];
        c->item_count--;
    }
    return item;
}

SolItem Sol_Inv_GetItemAtSlot(const ScInventory *inv, int slot)
{
    return inv->equipped_items[slot];
}