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

static int Ui_FindItemCard(World *hud, u32 world_id, u32 ent_id, u32 card_id)
{
    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind == REFKIND_ITEM && ref->ent_world == world_id && ref->ent_id == ent_id && ref->uid == card_id)
            return ref_id;
    }
    return -1;
}

void Ui_Sync_Inventory(World *hud, World *game, int user_ent)
{
    ScInventory *inv = Sol_Comp_Get(game, user_ent, ScInventory);
    if (!inv)
        return;
    SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int ref_id = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_ITEM || ref->ent_id != user_ent || ref->ent_world != game->index)
            continue;
        if (!Sol_Inv_GetItem_FromId(inv, ref->uid))
            Sol_Destroy_Ent(hud, ref_id);
    }
    for (int i = 0; i < inv->item_count; i++)
    {
        SolItem *item = &inv->items[i];
        if (Ui_FindItemCard(hud, game->index, user_ent, item->uid) >= 0)
            continue;
        sollog(i);
        vec3s pos = {
            100.0f + (i % 6) * 64.0f,
            100.0f + (i / 6) * 64.0f,
            0.0f,
        };
        int card_id                        = Sol_Prefab_AbilityCard(hud, pos, item);
        *Sol_Comp_Add(hud, card_id, ScRef) = (ScRef){
            .ent_id    = user_ent,
            .ent_world = game->index,
            .kind      = REFKIND_ITEM,
            .uid       = item->uid,
            .index     = i,
        };
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
        bool is_alive = (ref->ent_id == user_ent);
        if (!is_alive)
        {
            Sol_Destroy_Ent(hud, ref_id);
            continue;
        }
        if (ref->kind == REFKIND_HEALTHBAR)
            player_healthbar = true;
        else if (ref->kind == REFKIND_ABILITYBAR)
            player_abilitybar = true;
        else if (ref->kind == REFKIND_ENERGYBAR)
            player_energybar = true;
    }
    if (!player_healthbar && Sol_Comp_Has(game, user_ent, ScCombat))
    {
        float x          = 250.0f;
        float y          = 700.0f;
        int healthbar_id = Sol_Prefab_PowerVial(hud, (vec3s){x, y, 0}, (vec4s){0, 1, 0, 1});

        *Sol_Comp_Add(hud, healthbar_id, ScRef) = (ScRef){
            .kind      = REFKIND_HEALTHBAR,
            .ent_world = game->index,
            .ent_id    = user_ent,
        };
    }
    if (!player_energybar && Sol_Comp_Has(game, user_ent, ScCombat))
    {
        float x          = 900.0f;
        float y          = 700.0f;
        int energybar_id = Sol_Prefab_PowerVial(hud, (vec3s){x, y, 0}, (vec4s){1, 1, 0, 1});

        *Sol_Comp_Add(hud, energybar_id, ScRef) = (ScRef){
            .kind      = REFKIND_ENERGYBAR,
            .ent_world = game->index,
            .ent_id    = user_ent,
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
        int ent_id     = ref->ent_id;
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
                .kind      = REFKIND_HEALTHBAR,
                .ent_world = game->index,
                .ent_id    = player_ent,
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
                .kind      = REFKIND_ENERGYBAR,
                .ent_world = game->index,
                .ent_id    = player_ent,
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
