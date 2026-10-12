#include "world.h"
#include "sol_user.h"
#include "sol_math.h"
#include "sol_core.h"
#include "render/render.h"

void Sol_Ui_User_Update(World *hud, World *game, int user_ent)
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
            float x        = 0.0f;
            float y        = 50.0f + (p * 30.0f);
            if (player_ent == user_ent)
            {
                x = (WINDOW_WIDTH * 0.5f) - (300.0f * 0.5f);
                y = 610.0f;
            }
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
            float x        = 0.0f;
            float y        = 50.0f + (p * 30.0f);
            if (player_ent == user_ent)
            {
                x = (WINDOW_WIDTH * 0.5f) - (300.0f * 0.5f);
                y = 630.0f;
            }
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
            float x        = 200.0f;
            float y        = 50.0f + (p * 50.0f);
            if (player_ent == user_ent)
            {
                x = (WINDOW_WIDTH * 0.5f) - (434.0f * 0.5f);
                y = 700.0f;
            }
            float offset_x_group = x;
            int abilitybar_id0   = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group, y, 0}, player_ent, game->index,
                                                         (int[]){0, 1, 2, 3}, 4);
            int abilitybar_id1   = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group += 264.0f, y, 0}, player_ent,
                                                         game->index, (int[]){4}, 1);
            int abilitybar_id2   = Sol_Prefab_AbilityBar(hud, (vec3s){offset_x_group += 66.0f, y, 0}, player_ent,
                                                         game->index, (int[]){5, 6}, 2);
        }
    }
}