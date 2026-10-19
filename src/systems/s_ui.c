/*
 * File: s_ui.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-13
 *
 */
#include "world.h"
#include "sol_user.h"
#include "sol_math.h"
#include "sol_core.h"
#include "prefabs.h"
#include "render/render.h"

enum
{
    ITEM_LOOSE,
    ITEM_ABILITYBAR,
    ITEM_INVENTORY,
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
        World *game_world = Sol_GetWorldByIdx(ref->world);
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

                if (current_slots[s] != 0)
                {
                    continue;
                }

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
                Sol_Inv_Equip(game_world, ref->id, ability_slot, -1);
                continue;
            }

            ScRef *item_ref = Sol_Comp_Get(world, new_item, ScRef);
            if (!item_ref || item_ref->kind != REFKIND_ITEM)
                continue;
            forc(game_world, ref->id, ScInventory)
            {
                Sol_Inv_Equip(game_world, ref->id, ability_slot, item_ref->index);
                item_ref->flags = ITEM_ABILITYBAR;
            }
        }
    }
}

void Inventory_Update(World *world, double dt)
{
    SparseSet_ScRef *ref_set = Sol_Comp_Set(world, ScRef);
    for (int i = 0; i < ref_set->cnt; i++)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_ITEM)
            continue;

        bool dragging                            = false;
        forc(world, ref_id, ScInteract) dragging = (c->state & (1u << INTERACT_DRAGGING));
        if (dragging)
        {
            Sol_Comp_Rem(world, ref_id, ScParent);
            ref->flags = ITEM_LOOSE;
            continue;
        }
        if (Sol_Comp_Has(world, ref_id, ScParent))
        {
            ref->flags = ITEM_INVENTORY;
            continue;
        }

        int target, target_kind = -1;
        int ids[4];
        int hits = Sol_Body2_GetOverlaps(world, ref_id, ids, 4);
        for (int hit = 0; hit < hits; hit++)
        {
            ScRef *r = Sol_Comp_Get(world, ids[hit], ScRef);
            if (!r)
                continue;
            if (r->kind == REFKIND_ABILITYBAR)
            {
                target      = ids[hit];
                target_kind = REFKIND_ABILITYBAR;
                break;
            }
            if (r->kind == REFKIND_INVENTORY)
            {
                target      = ids[hit];
                target_kind = REFKIND_INVENTORY;
            }
        }

        switch (target_kind)
        {
        case REFKIND_ABILITYBAR: {
            ref->flags = ITEM_ABILITYBAR;
        }
        break;
        case REFKIND_INVENTORY: {
            ref->flags                             = ITEM_INVENTORY;
            *Sol_Comp_Add(world, ref_id, ScParent) = (ScParent){
                .parentId    = target,
                .localOffset = vecSub(world->xform.pos[ref_id], world->xform.pos[target]),
            };
        }
        break;
        default:
            ref->flags = ITEM_LOOSE;
        }
    }
}

static int Ui_FindItemCard(World *hud, u32 world_id, u32 ent_id, u32 card_id)
{
    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind == REFKIND_ITEM && ref->world == world_id && ref->id == ent_id && ref->uid == card_id)
            return ref_id;
    }
    return -1;
}

void Sol_Ui_Sync_AbilityItems(World *hud, World *game, int user_ent)
{
    ScInventory *inv = Sol_Comp_Get(game, user_ent, ScInventory);
    if (!inv)
        return;
    inv->dirty_equip              = false;
    vec3s slot_pos[ABILITY_SLOTS] = {0};

    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->world != game->index)
        {
            Sol_Destroy_Ent(hud, ref_id);
            continue;
        }
        switch (ref->kind)
        {
        case REFKIND_ABILITYBAR: {
            forc(hud, ref_id, ScAbilitybar)
            {
                vec3s bar_pos = hud->xform.pos[ref_id];
                for (int j = 0; j < c->slot_count; j++)
                {
                    int s        = c->ability_slots[j];
                    vec3s offset = bar_pos;
                    offset.x += j * c->slot_dims.x;
                    slot_pos[s] = offset;
                }
            }
        }
        break;
        case REFKIND_ITEM: {
            if (!Sol_Inv_GetItem_FromId(inv, ref->uid))
                Sol_Destroy_Ent(hud, ref_id);
        }
        break;
        }
    }
    int equipped_count = 0;
    for (int j = 0; j < ABILITY_SLOTS; j++)
    {
        int equip_idx = inv->equipped_items[j];
        if (inv->equipped_items[j] >= 0)
        {
            equipped_count++;
            SolItem *item = &inv->items[equip_idx];
            if (Ui_FindItemCard(hud, game->index, user_ent, item->uid) >= 0)
                continue;
            int card_id     = Sol_Prefab_AbilityCard(hud, GLMS_VEC3_ZERO, item);
            ScRef *card_ref = Sol_Comp_Add(hud, card_id, ScRef);
            *card_ref       = (ScRef){
                .id    = user_ent,
                .world = game->index,
                .kind  = REFKIND_ITEM,
                .uid   = item->uid,
                .index = equip_idx,
                .flags = ITEM_ABILITYBAR,
            };
            hud->xform.pos[card_id] = slot_pos[j];
        }
    }

    // int item_count = solb_count(inv->items);
    // for (int i = 0; i < item_count; i++)
    // {
    //     SolItem *item = &inv->items[i];
    //     if (Ui_FindItemCard(hud, game->index, user_ent, item->uid) >= 0)
    //         continue;
    //     for (int j = 0; j < ABILITY_SLOTS; j++)
    //     {
    //         if (inv->equipped_items[j] == i)
    //         {
    //             int card_id = Sol_Prefab_AbilityCard(hud, GLMS_VEC3_ZERO, item);

    //             ScRef *card_ref = Sol_Comp_Add(hud, card_id, ScRef);
    //             *card_ref       = (ScRef){
    //                 .id    = user_ent,
    //                 .world = game->index,
    //                 .kind      = REFKIND_ITEM,
    //                 .uid       = item->uid,
    //                 .index     = i,
    //                 .flags     = ITEM_ABILITYBAR,
    //             };
    //             vec3s *pos = &hud->xform.pos[card_id];
    //             *pos       = slot_pos[j];
    //         }
    //     }
    // }
}

// void Sol_Ui_RemoveStaleRefs(World *hud, World *game, int user_ent)
// {
//     SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
//     ScInventory *inv = Sol_Comp_Get(game, user_ent, ScInventory);
//     for (int i = ref_set->cnt - 1; i >= 0; i--)
//     {
//         int ref_id = ref_set->dense[i];
//         ScRef *ref = &ref_set->data[i];
//         switch (ref->kind)
//         {
//         case REFKIND_ABILITYBAR: {
//             forc(hud, ref_id, ScAbilitybar)
//             {
//                 vec3s bar_pos = hud->xform.pos[ref_id];
//                 for (int j = 0; j < c->slot_count; j++)
//                 {
//                     int s        = c->ability_slots[j];
//                     vec3s offset = bar_pos;
//                     offset.x += j * c->slot_dims.x;
//                     slot_pos[s] = offset;
//                 }
//             }
//         }
//         break;
//         case REFKIND_INVENTORY: {
//             inv_window_pos = hud->xform.pos[ref_id];
//             inv_id         = ref_id;
//         }
//         break;
//         case REFKIND_ITEM: {
//             if (!Sol_Inv_GetItem_FromId(inv, ref->uid))
//                 Sol_Destroy_Ent(hud, ref_id);
//         }
//         break;
//         }
//     }

// }

void Sol_Ui_Sync_Inventory(World *hud, World *game, int user_ent, vec3s inv_pos)
{
    ScInventory *inv = Sol_Comp_Get(game, user_ent, ScInventory);
    if (!inv)
        return;
    inv->dirty                    = false;
    inv->dirty_equip              = false;
    vec3s slot_pos[ABILITY_SLOTS] = {0};
    int inv_id                    = 0;
    vec3s inv_window_pos          = inv_pos;
    int grid_n                    = 0;

    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        switch (ref->kind)
        {
        case REFKIND_ABILITYBAR: {
            forc(hud, ref_id, ScAbilitybar)
            {
                vec3s bar_pos = hud->xform.pos[ref_id];
                for (int j = 0; j < c->slot_count; j++)
                {
                    int s        = c->ability_slots[j];
                    vec3s offset = bar_pos;
                    offset.x += j * c->slot_dims.x;
                    slot_pos[s] = offset;
                }
            }
        }
        break;
        case REFKIND_INVENTORY: {
            inv_window_pos = hud->xform.pos[ref_id];
            inv_id         = ref_id;
        }
        break;
        case REFKIND_ITEM: {
            if (!Sol_Inv_GetItem_FromId(inv, ref->uid))
                Sol_Destroy_Ent(hud, ref_id);
            else if (ref->flags == ITEM_INVENTORY)
                grid_n++;
        }
        break;
        }
    }
    if (!inv_id)
    {
        inv_window_pos = inv_pos;
        inv_id         = Sol_Prefab_InventoryWindow(hud, inv_window_pos);

        *Sol_Comp_Add(hud, inv_id, ScRef) = (ScRef){
            .kind  = REFKIND_INVENTORY,
            .id    = user_ent,
            .world = game->index,
        };
    }

    int item_count        = solb_count(inv->items);
    int equipped_count    = 0;
    int docked_item_count = 0;
    for (int i = 0; i < item_count; i++)
    {
        bool equipped = false;
        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            if (inv->equipped_items[j] == i)
            {
                equipped_count++;
                equipped = true;
                break;
            }
        }
        if (!equipped)
            docked_item_count++;
    }
    {
        int rows     = (docked_item_count + 5) / 6;
        float height = 30.0f + 64.0f * rows;
        float width  = 30.0f + 64.0f * 6;
        forc(hud, inv_id, ScBody2)
        {
            c->dims.x = width;
            c->dims.y = height;
        }
    }
    for (int i = 0; i < item_count; i++)
    {
        SolItem *item = &inv->items[i];
        // Skip already spawned cards
        if (Ui_FindItemCard(hud, game->index, user_ent, item->uid) >= 0)
            continue;
        int bar_slot = -1;
        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            if (inv->equipped_items[j] == i)
            {
                bar_slot = j;
                break;
            }
        }
        int card_id = Sol_Prefab_AbilityCard(hud, GLMS_VEC3_ZERO, item);

        ScRef *card_ref = Sol_Comp_Add(hud, card_id, ScRef);
        *card_ref       = (ScRef){
            .id    = user_ent,
            .world = game->index,
            .kind  = REFKIND_ITEM,
            .uid   = item->uid,
            .index = i,
        };
        vec3s *pos = &hud->xform.pos[card_id];
        if (bar_slot >= 0)
        {
            card_ref->flags = ITEM_ABILITYBAR;
            *pos            = slot_pos[bar_slot];
        }
        else
        {
            card_ref->flags = ITEM_INVENTORY;
            *pos            = (vec3s){
                15.0f + (grid_n % 6) * 64.0f,
                15.0f + (grid_n / 6) * 64.0f,
                0.0f,
            };
            *Sol_Comp_Add(hud, card_id, ScParent) = (ScParent){
                .localOffset = *pos,
                .parentId    = inv_id,
            };
            grid_n++;
        };
    }
}

void Sol_Ui_CloseInv(World *hud, vec3s *inv_pos)
{
    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        switch (ref->kind)
        {
        case REFKIND_ITEM: {
            if (ref->flags != ITEM_ABILITYBAR)
                Sol_Destroy_Ent(hud, ref_id);
        }
        break;
        case REFKIND_INVENTORY: {
            *inv_pos = hud->xform.pos[ref_id];
            Sol_Destroy_Ent(hud, ref_id);
        }
        break;
        }
    }
}

void Sol_Ui_User_Update(World *hud, World *game, int user_ent)
{
    if (!hud || !game)
        return;
    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);

    bool player_healthbar  = 0;
    bool player_energybar  = 0;
    bool player_abilitybar = 0;
    // Cleanup UI from destroyed player ents
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_HEALTHBAR && ref->kind != REFKIND_ABILITYBAR && ref->kind != REFKIND_ENERGYBAR)
            continue;
        int ref_id    = ref_set->dense[i];
        bool is_alive = (ref->id == user_ent && ref->world == game->index);
        if (!is_alive)
        {
            Sol_Destroy_Ent(hud, ref_id);
            continue;
        }
        switch (ref->kind)
        {
        case REFKIND_HEALTHBAR: {
            player_healthbar = true;
            ScView2 *view2   = Sol_Comp_Get(hud, ref_id, ScView2);
            ScCombat *combat = Sol_Comp_Get(game, ref->id, ScCombat);
            if (combat && view2)
                snprintf(view2->views[5].text, sizeof(view2->views[5].text), "%.0f", combat->health);
        }
        break;
        case REFKIND_ABILITYBAR: {
            player_abilitybar = true;
        }
        break;
        case REFKIND_ENERGYBAR: {
            player_energybar = true;
            ScView2 *view2   = Sol_Comp_Get(hud, ref_id, ScView2);
            ScCombat *combat = Sol_Comp_Get(game, ref->id, ScCombat);
            if (combat && view2)
                snprintf(view2->views[5].text, sizeof(view2->views[5].text), "%.0f", combat->energy);
        }
        break;
        }
    }
    if (!player_healthbar && Sol_Comp_Has(game, user_ent, ScCombat))
    {
        float x          = 250.0f;
        float y          = 700.0f;
        int healthbar_id = Sol_Prefab_PowerVial(hud, (vec3s){x, y, 0}, (vec4s){0, 1, 0, 1});

        *Sol_Comp_Add(hud, healthbar_id, ScRef) = (ScRef){
            .kind  = REFKIND_HEALTHBAR,
            .world = game->index,
            .id    = user_ent,
        };
    }
    if (!player_energybar && Sol_Comp_Has(game, user_ent, ScCombat))
    {
        float x          = 900.0f;
        float y          = 700.0f;
        int energybar_id = Sol_Prefab_PowerVial(hud, (vec3s){x, y, 0}, (vec4s){1, 1, 0, 1});

        *Sol_Comp_Add(hud, energybar_id, ScRef) = (ScRef){
            .kind  = REFKIND_ENERGYBAR,
            .world = game->index,
            .id    = user_ent,
        };
    }
    if (!player_abilitybar && Sol_Comp_Has(game, user_ent, ScAbility))
    {
        float x              = 400.0f;
        float y              = 700.0f;
        float offset_x_group = x;

        forc(game, user_ent, ScAbility)
        {

            int abilitybar_id0 = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group, y, 0}, user_ent, game->index,
                                                       (int[]){0, 1, 2, 3}, 4);

            int abilitybar_id1 = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group += 264.0f, y, 0}, user_ent,
                                                       game->index, (int[]){4}, 1);

            int abilitybar_id2 = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group += 66.0f, y, 0}, user_ent,
                                                       game->index, (int[]){5, 6}, 2);
        }
    }
}

void Sol_Ui_PlayerWorldOverlay(World *hud, World *game)
{
    if (!hud || !game)
        return;
    SparseSet_ScRef *ref_set       = Sol_Comp_Set(hud, ScRef);
    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game, ScPlayer);

    bool player_healthbars[32]  = {0};
    bool player_energybars[32]  = {0};
    bool player_abilitybars[32] = {0};
    // Cleanup UI from destroyed player ents
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_HEALTHBAR && ref->kind != REFKIND_ABILITYBAR && ref->kind != REFKIND_ENERGYBAR)
            continue;
        int ref_id     = ref_set->dense[i];
        int ent_id     = ref->id;
        int player_idx = player_set->sparse[ent_id];
        bool is_alive  = (player_idx >= 0 && player_idx < player_set->cnt && player_set->dense[player_idx] == ent_id);
        if (!is_alive)
        {
            Sol_Destroy_Ent(hud, ref_id);
            continue;
        }
        if (player_idx < 32)
        {
            if (ref->kind == REFKIND_HEALTHBAR)
                player_healthbars[player_idx] = true;
            else if (ref->kind == REFKIND_ABILITYBAR)
                player_abilitybars[player_idx] = true;
            else if (ref->kind == REFKIND_ENERGYBAR)
                player_energybars[player_idx] = true;
        }
    }
    for (int p = 0; p < player_set->cnt; p++)
    {
        if (p < 32 && !player_healthbars[p])
        {
            int player_ent = player_set->dense[p];
            if (!Sol_Comp_Has(game, player_ent, ScCombat))
                continue;

            float x          = 0.0f;
            float y          = 50.0f + (p * 30.0f);
            int healthbar_id = Sol_Prefab_PowerVial(hud, (vec3s){x, y, 0}, (vec4s){0, 1, 0, 1});

            *Sol_Comp_Add(hud, healthbar_id, ScRef) = (ScRef){
                .kind  = REFKIND_HEALTHBAR,
                .world = game->index,
                .id    = player_ent,
            };
        }
    }
    for (int p = 0; p < player_set->cnt; p++)
    {
        if (p < 32 && !player_energybars[p])
        {
            int player_ent = player_set->dense[p];
            if (!Sol_Comp_Has(game, player_ent, ScCombat))
                continue;
            float x          = 0.0f;
            float y          = 50.0f + (p * 30.0f);
            int energybar_id = Sol_Prefab_PowerVial(hud, (vec3s){x, y, 0}, (vec4s){1, 1, 0, 1});

            *Sol_Comp_Add(hud, energybar_id, ScRef) = (ScRef){
                .kind  = REFKIND_ENERGYBAR,
                .world = game->index,
                .id    = player_ent,
            };
        }
    }
    for (int p = 0; p < player_set->cnt; p++)
    {
        if (p < 32 && !player_abilitybars[p])
        {
            int player_ent = player_set->dense[p];
            if (!Sol_Comp_Has(game, player_ent, ScAbility))
                continue;
            float x              = 200.0f;
            float y              = 50.0f + (p * 50.0f);
            float offset_x_group = x;

            forc(game, player_ent, ScAbility)
            {

                int abilitybar_id0 = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group, y, 0}, player_ent, game->index,
                                                           (int[]){0, 1, 2, 3}, 4);

                int abilitybar_id1 = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group += 264.0f, y, 0}, player_ent,
                                                           game->index, (int[]){4}, 1);

                int abilitybar_id2 = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group += 66.0f, y, 0}, player_ent,
                                                           game->index, (int[]){5, 6}, 2);
            }
        }
    }
}

void Sol_Ui_Sync_Rules(World *hud, World *game, double dt)
{
    bool existing_window = false;
    for_set(hud, ScRef)
    {
        int id = set->dense[i];
        switch (c->kind)
        {
        case REFKIND_RULESWINDOW: {
            existing_window = true;
        }
        break;
        }
    }
    if (!existing_window)
    {
        int window_id = Sol_Prefab_RulesWindow(hud, (vec3s){0, 0, 0});
        ScRef *ref    = Sol_Comp_Add(hud, window_id, ScRef);
        ref->kind     = REFKIND_RULESWINDOW;
        ref->world    = game->index;
    }
}

void Ui_Update(World *world, double dt)
{
    Abilitybar_Update(world, dt);
    Inventory_Update(world, dt);
}