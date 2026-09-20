#include "world.h"
#include "sol_core.h"
#include "sol_user.h"
#include "sol_math.h"

void Abilitybar_Update(World *world)
{
    SparseSet_ScAbilitybar *set = Sol_Comp_Set(world, ScAbilitybar);
    for (int i = 0; i < set->cnt; i++)
    {
        int id                   = set->dense[i];
        ScAbilitybar *abilitybar = &set->data[i];
        ScInteract *interact     = Sol_Comp_Get(world, id, ScInteract);
        Xform xform              = Xform_Get(world, id);
        ScRef *ref               = Sol_Comp_Get(world, id, ScRef);
        if (!ref)
            continue;

        World *game_world    = Sol_GetWorldByIdx(ref->ent_world);
        ScAbility *abilities = Sol_Comp_Get(game_world, ref->ent_id, ScAbility);
        if (abilities)
        {
            memset(abilities->slotted_actions, 0, sizeof(abilities->slotted_actions));
        }

        float slot_w  = abilitybar->slot_dims.x;
        float slot_h  = abilitybar->slot_dims.y;
        float half_w  = slot_w * 0.5f;
        float half_h  = slot_h * 0.5f;
        int count     = abilitybar->slots <= 12 ? abilitybar->slots : 12;
        float total_w = slot_w * count;

        // Precompute 3D slot centers
        vec3s slot_centers[12];
        for (int j = 0; j < count; j++)
        {
            slot_centers[j] = (vec3s){
                .x = xform.pos.x + (slot_w * j) + half_w,
                .y = xform.pos.y + half_h,
                .z = xform.pos.z,
            };
        }

        // --- OVERLAPPING ITEM SNAPPING ---
        int ids[8] = {0};
        int hits   = Sol_Body2_GetOverlaps(world, id, ids, 8);
        for (int j = 0; j < hits; j++)
        {
            int item_id     = ids[j];
            ScRef *item_ref = Sol_Comp_Get(world, item_id, ScRef);

            if (item_ref && item_ref->kind == REFKIND_ITEM)
            {
                Xform item_xform   = Xform_Get(world, item_id);
                ScBody2 *item_body = Sol_Comp_Get(world, item_id, ScBody2);

                float item_half_w = item_body->dims.x * 0.5f;
                float item_half_h = item_body->dims.y * 0.5f;

                // Center of the item relative to ability bar origin
                float rel_x = (item_xform.pos.x + item_half_w) - xform.pos.x;
                float rel_y = (item_xform.pos.y + item_half_h) - xform.pos.y;

                // Strict AABB check against the entire bar bounds
                if (rel_x >= 0.0f && rel_x < total_w && rel_y >= 0.0f && rel_y <= slot_h)
                {
                    int slot = (int)(rel_x / slot_w);
                    if (slot < count)
                    {
                        // Target item top-left = slot_center - item_half_dims
                        vec3s target_pos = {.x = slot_centers[slot].x - item_half_w,
                                            .y = slot_centers[slot].y - item_half_h,
                                            .z = item_xform.pos.z};

                        // Magnetic velocity pull toward target top-left
                        vec3s diff     = glms_vec3_sub(target_pos, item_xform.pos);
                        item_body->vel = glms_vec3_scale(diff, 12.0f);

                        SolItem *user_item = &user_data.items[item_ref->index];
                        if (user_item)
                        {                            
                            abilities->slotted_actions[slot] = user_item->kind;
                            abilities->slotted_items[slot]   = *user_item;
                        }
                    }
                }
            }
        }

        // --- CLICK DETECTION ---
        if (interact->state & INTERACT_JUSTDOWN)
        {
            float rel_x = interact->down_pos.x - xform.pos.x;
            float rel_y = interact->down_pos.y - xform.pos.y;

            if (rel_x >= 0.0f && rel_x < total_w && rel_y >= 0.0f && rel_y <= slot_h)
            {
                int slot = (int)(rel_x / slot_w);
                if (slot < count)
                {
                    sollog(slot);
                }
            }
        }
    }
}