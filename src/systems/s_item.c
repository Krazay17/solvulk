/*
 * File: s_item.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-30
 *
 */
#include "world.h"
#include "sol_math.h"
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

void Sol_Inv_AllDirty(World *world)
{
    SparseSet_ScInventory *inv_set = Sol_Comp_Set(world, ScInventory);
    for (int i = 0; i < inv_set->cnt; i++)
    {
        int id           = inv_set->dense[i];
        ScInventory *inv = &inv_set->data[i];
        inv->dirty       = true;
        inv->dirty_equip = true;
    }
}

void Item_Update(World *world, double dt)
{
    // SparseSet_ScInventory *inv_set = Sol_Comp_Set(world, ScInventory);
    // for (int i = 0; i < inv_set->cnt; i++)
    // {
    //     int id           = inv_set->dense[i];
    //     ScInventory *inv = &inv_set->data[i];
    //     if (inv->dirty_equip)
    //     {
    //         for (int j = 0; j < ABILITY_SLOTS; j++)
    //         {
    //             forc(world, id, ScEquip)
    //             {
    //                 if (!c->left_weapon || !c->right_weapon)
    //                     Sol_Equip_Item(world, id, inv->items[inv->equipped_items[j]].item_kind, j);
    //             }
    //         }
    //     }
    // }
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
    World *ref_world = Sol_GetWorldByIdx(ref->world);
    if (!ref_world)
        return item;
    forc(ref_world, ref->id, ScInventory)
    {
        item = Sol_Inv_GetItem_FromId(c, ref->uid);
    }

    return item;
}

void Sol_Inv_Equip(World *world, int id, int slot, int idx)
{
    forc(world, id, ScInventory)
    {
        c->equipped_items[slot] = idx;
        Sol_Equip_Item(world, id, idx >= 0 ? c->items[idx].item_kind : 0, slot);
        c->dirty_equip = true;
    }
}

SolItem Sol_Inv_Remove(World *world, int id, u32 idx)
{
    SolItem item = {0};
    forc(world, id, ScInventory)
    {
        item      = c->items[idx];
        int count = solb_count(c->items);
        if (idx < count)
        {
            c->items[idx] = c->items[count - 1];
            solb_pop(c->items);
        }
    }
    return item;
}

SolItem Sol_Inv_GetItemAtSlot(const ScInventory *inv, int slot)
{
    return inv->items[inv->equipped_items[slot]];
}

SolItem *Sol_Inv_GetItem_FromId(ScInventory *inv, u32 uid)
{
    for (int i = 0; i < solb_count(inv->items); i++)
    {
        SolItem *item = &inv->items[i];
        if (item->uid == uid)
            return item;
    }
    return NULL;
}
int Sol_Inv_AddItem(World *world, int id, SolItem *item, bool try_equip)
{
    ScInventory *c = Sol_Comp_Add(world, id, ScInventory);
    if (!c)
        return 0;

    int idx = 0;
    if (c->next_item_id == 0)
        c->next_item_id = 1;
    item->uid = c->next_item_id++;
    idx       = solb_count(c->items);
    solb_push(c->items, *item);
    c->dirty = true;

    if (try_equip)
    {
        for (int i = 0; i < ABILITY_SLOTS; i++)
        {
            if (c->equipped_items[i] <= -1)
            {
                Sol_Inv_Equip(world, id, i, idx);
                break;
            }
        }
    }

    return idx;
}
int Sol_Inv_RemItem(World *world, int id, u32 idx)
{
    return 0;
}