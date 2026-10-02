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
        ScAbility *abilities = Sol_Comp_Get(game_world, ref->ent_id, ScAbility);
        if (!abilities)
            continue;

        memset(abilities->slotted_actions, 0, sizeof(abilities->slotted_actions));
        memset(abilitybar->slotted_ents, 0, sizeof(abilitybar->slotted_ents));

        // Same layout the view uses, offset by the bar's position
        vec3s origin = Xform_Get(world, id).pos;
        vec4s slots[ABILITY_SLOTS], frames[ABILITY_GROUPS];
        int total_slots = Abilitybar_Layout(abilitybar, slots, frames);

        int ids[8] = {0};
        int hits   = Sol_Body2_GetOverlaps(world, id, ids, 8);
        for (int j = 0; j < hits; j++)
        {
            int item_id     = ids[j];
            ScRef *item_ref = Sol_Comp_Get(world, item_id, ScRef);
            if (!item_ref || item_ref->kind != REFKIND_ITEM)
                continue;

            Xform item_xform   = Xform_Get(world, item_id);
            ScBody2 *item_body = Sol_Comp_Get(world, item_id, ScBody2);
            vec2s item_half    = {item_body->dims.x * 0.5f, item_body->dims.y * 0.5f};
            vec2s item_center  = {item_xform.pos.x + item_half.x, item_xform.pos.y + item_half.y};

            int target = -1;
            for (int s = 0; s < total_slots; s++)
            {
                float sx = origin.x + slots[s].x;
                float sy = origin.y + slots[s].y;
                if (item_center.x < sx || item_center.x > sx + slots[s].z ||
                    item_center.y < sy || item_center.y > sy + slots[s].w)
                    continue;

                if (abilitybar->slotted_ents[s] == 0 || abilitybar->slotted_ents[s] == item_id)
                    target = s;
                break;
            }
            if (target == -1)
                continue;

            abilitybar->slotted_ents[target] = item_id;

            vec3s target_pos = {
                origin.x + slots[target].x + slots[target].z * 0.5f - item_half.x,
                origin.y + slots[target].y + slots[target].w * 0.5f - item_half.y,
                item_xform.pos.z,
            };
            item_body->vel = glms_vec3_scale(glms_vec3_sub(target_pos, item_xform.pos), 12.0f);

            SolItem *user_item = &user_data.items[item_ref->index];
            abilities->slotted_actions[target] = user_item->abilityKind;
            abilities->slotted_items[target]   = *user_item;
        }
    }
}