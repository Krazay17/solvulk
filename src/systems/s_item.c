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

void Abilitybar_Update(World *world, double dt)
{
    SparseSet_ScAbilitybar *set = Sol_Comp_Set(world, ScAbilitybar);
    for (int i = 0; i < set->cnt; i++)
    {
        int id                   = set->dense[i];
        ScAbilitybar *abilitybar = &set->data[i];
        ScRef *ref               = Sol_Comp_Get(world, id, ScRef);
        if (!ref)
            continue;
        World *game_world = Sol_GetWorldByIdx(ref->ent_world);
        if (!game_world)
            continue;

        vec3s origin    = world->xform.pos[id];
        vec2s slot_dims = abilitybar->slot_dims;

        int current_slots[ABILITY_SLOTS] = {0};
        int ids[8]                       = {0};
        int hits                         = Sol_Body2_GetOverlaps(world, id, ids, 8);
        for (int j = 0; j < hits; j++)
        {
            int item_id     = ids[j];
            ScRef *item_ref = Sol_Comp_Get(world, item_id, ScRef);
            if (!item_ref || item_ref->kind != REFKIND_ITEM)
                continue;

            vec3s item_pos     = world->xform.pos[item_id];
            ScBody2 *item_body = Sol_Comp_Get(world, item_id, ScBody2);
            vec2s item_half    = {item_body->dims.x * 0.5f, item_body->dims.y * 0.5f};
            vec2s item_center  = {item_pos.x + item_half.x, item_pos.y + item_half.y};

            int target = -1;
            for (int s = 0; s < abilitybar->slot_count; s++)
            {
                float sx = origin.x + abilitybar->slot_dims.x * s;
                float sy = origin.y;
                if (item_center.x < sx || item_center.x > sx + slot_dims.x || item_center.y < sy ||
                    item_center.y > sy + slot_dims.y)
                    continue;

                if (current_slots[s] == 0)
                    target = s;
                break;
            }
            if (target == -1)
                continue;
            current_slots[target] = item_id;

            vec2s slot_center = {origin.x + slot_dims.x * target + slot_dims.x * 0.5f, origin.y + slot_dims.y * 0.5f};
            vec3s delta       = {slot_center.x - item_center.x, slot_center.y - item_center.y};
            item_body->vel    = glms_vec3_scale(delta, 12.0f);
        }

        for (int s = 0; s < abilitybar->slot_count; s++)
        {
            int old_item     = abilitybar->slotted_ents[s];
            int new_item     = current_slots[s];
            int ability_slot = abilitybar->ability_slots[s];

            if (old_item == new_item)
                continue;

            abilitybar->slotted_ents[s] = new_item;

            if (new_item == 0)
            {
                Sol_Inv_Equip(game_world, ref->ent_id, ability_slot, -1);
                continue;
            }

            ScRef *item_ref = Sol_Comp_Get(world, new_item, ScRef);
            if (!item_ref || item_ref->kind != REFKIND_ITEM)
                continue;
            forc(game_world, ref->ent_id, ScInventory)
            {
                Sol_Inv_Equip(game_world, ref->ent_id, ability_slot, item_ref->index);
            }
        }
    }
}

bool g_items_active[256] = {0};
void Item_User_Update(World *world, SolItem *items, int count, u32 kind)
{
    SparseSet_ScRef *ref_set = Sol_Comp_Set(world, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int id     = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != kind)
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
            if (!item->item_kind)
                continue;
            float x     = 0.0f + (i % 5) * 64.0f;
            float y     = 0.0f + (i / 5) * 64.0f;
            int card_id = Sol_Prefab_AbilityCard(world, (vec3s){x, y, 0}, item);

            *Sol_Comp_Add(world, card_id, ScRef) = (ScRef){.kind = kind, .index = i};
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

void Sol_Inv_Equip(World *world, int id, int slot, int idx)
{
    // forc(world, id, ScUser)
    // {
    //     user_data.inventory.equipped_items[slot] = item;
    //     return;
    // }
    forc(world, id, ScInventory)
    {
        c->equipped_items[slot] = idx;
        Sol_Equip_Item(world, id, idx >= 0 ? c->items[idx].item_kind : 0, slot);
    }
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
    return inv->items[inv->equipped_items[slot]];
}

SolItem *Sol_Inv_GetItem_FromId(ScInventory *inv, u32 uid)
{
    for (int i = 0; i < inv->item_count; i++)
    {
        SolItem *item = &inv->items[i];
        if (item->uid == uid)
            return item;
    }
    return NULL;
}
