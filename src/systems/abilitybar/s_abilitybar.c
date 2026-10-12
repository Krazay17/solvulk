/*
 * File: s_abilitybar.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-10
 *
 */
#include "world.h"
#include "sol_core.h"
#include "sol_user.h"
#include "sol_math.h"
#include "ability/s_ability.h"
#include "s_abilitybar.h"

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
            if (!item_ref || item_ref->kind != REFKIND_LOOSEITEM)
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
                Sol_Ability_Equip(game_world, ref->ent_id, ability_slot, NULL);
                continue;
            }

            ScRef *item_ref = Sol_Comp_Get(world, new_item, ScRef);
            if (!item_ref || item_ref->kind != REFKIND_LOOSEITEM)
                continue;

            SolItem *user_item = &user_data.loose_items[item_ref->index];

            Sol_Ability_Equip(game_world, ref->ent_id, ability_slot, user_item);
        }
    }
}