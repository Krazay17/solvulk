/*
 * File: s_hud.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-25
 *
 */

#include "s_hud.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "game/prefabs.h"

#include "xform/s_xform.h"
#include "player/s_player.h"
#include "ability/s_ability.h"
#include "inventory/s_inventory.h"
#include "interact/s_interact.h"
#include "physx/s_body2d.h"

const u32 ability_icon[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_CLAW] = SOL_TEXTURE_BLADE_CARD,        [ABILITY_STATE_DASH] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_FIREBALL] = SOL_TEXTURE_FIREBALL_CARD, [ABILITY_STATE_PISTOL] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_SPINSLASH] = SOL_TEXTURE_BLADE_CARD,   [ABILITY_STATE_CLAW] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_SHIELD] = SOL_TEXTURE_BLADE_CARD,      [ABILITY_STATE_LASER] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_WHIP] = SOL_TEXTURE_BLADE_CARD,        [ABILITY_STATE_FIREBALLVOLLEY] = SOL_TEXTURE_BLADE_CARD,
};

typedef struct
{
    int          cnt, cap;
    int         *sparse, *dense;
    CompHudItem *hud_items;

    int          slot_cnt, slot_cap;
    int         *slot_sparse, *slot_dense;
    CompHudSlot *hud_slots;

    int last_target_ent;
} WorldHuds;

static void Slot_Tick(World *world, double dt, double time)
{
    WorldHuds *wc = world->dense_components[WORLD_SYS_HUD];
    if (!wc)
        return;
    World *activeWorld = Sol_GetActiveGameWorld();
    int    playerId    = Sol_Player_GetEnt(activeWorld, 0);
    if (playerId < 0)
        return;
    CompInventory *inv = Sol_Inventory_Get(activeWorld, playerId);

    for (int i = 0; i < wc->slot_cnt; i++)
    {
        int          id         = wc->dense[i];
        CompHudSlot *hud_slot   = &wc->hud_slots[i];
        CompBody2d  *slot_body  = Sol_Body2d_Get(world, id);
        vec3s        slotPos    = Sol_Xform_GetPos(world, id);
        vec2s        slotCenter = {slotPos.x + (slot_body->dims.x * 0.5f), slotPos.y + (slot_body->dims.y * 0.5f)};

        int   bestCardIdx   = -1;
        int   bestCardId    = -1;
        float minDistanceSq = 9999999.0f;
        int   overlappingIds[8];
        int   count = Sol_Body2d_GetOverlapping(world, id, overlappingIds, 8);
        for (int j = 0; j < count; j++)
        {
            int          overlappingId = overlappingIds[j];
            CompHudItem *hud_item      = Sol_Hud_GetItem(world, overlappingId);
            if (hud_item)
            {
                CompBody2d *cardBody = Sol_Body2d_Get(world, overlappingId);
                vec3s       cardPos  = Sol_Xform_GetPos(world, overlappingId);

                // Find the center of the card rectangle
                vec2s cardCenter = {cardPos.x + (cardBody->dims.x * 0.5f), cardPos.y + (cardBody->dims.y * 0.5f)};

                // Calculate squared distance (skipping sqrt for performance)
                float dx     = cardCenter.x - slotCenter.x;
                float dy     = cardCenter.y - slotCenter.y;
                float distSq = (dx * dx) + (dy * dy);

                // The closest card to the absolute center of this slot wins ownership
                if (distSq < minDistanceSq)
                {
                    minDistanceSq = distSq;
                    bestCardIdx   = hud_item->idx;
                    bestCardId    = overlappingId;
                }
                if (bestCardIdx >= 0)
                {
                    vec3s       cardPos    = Sol_Xform_GetPos(world, bestCardId);
                    CompBody2d *cardBody   = Sol_Body2d_Get(world, bestCardId);
                    float       halfWidth  = cardBody->dims.x * 0.5f;
                    float       halfHeight = cardBody->dims.y * 0.5f;
                    vec2s       cardCenter = {cardPos.x + (halfWidth), cardPos.y + (halfHeight)};

                    float dx          = slotCenter.x - cardCenter.x;
                    float dy          = slotCenter.y - cardCenter.y;
                    vec2s anchorVel   = world->body2d[Sol_Parent_GetParent(world, id)].vel;
                    float anchorSpeed = glms_vec2_norm2(anchorVel);

                    if (!(Sol_Interact_Get(world, bestCardId)->state & INTERACT_DRAGGING))
                    {
                        cardBody->vel.x = fmaxf(-5.0f, fminf(5.0f, dx));
                        cardBody->vel.y = fmaxf(-5.0f, fminf(5.0f, dy));
                    }
                }
            }
        }
        inv->abilityBar[hud_slot->slot] = bestCardIdx;
    }
    // static int    required  = BITC(HAS_ACTIVE) | BITC(HAS_BODY2) | BITC(HAS_VIEW2D);
    // CompBody2d   *body2ds   = world->body2d;
    // CompInteract *interacts = world->interacts;
    // CompView2d   *view2ds   = world->view2d;

    // for (int i = 0; i < world->activeCount; i++)
    // {
    //     int id = world->activeEntities[i];
    //     if (!WHas(world, id, required))
    //         continue;

    //     // CompAbilitySlot *abilitySlot = &abilitySlots[id];
    //     CompBody2d *body = &body2ds[id];

    //     // Find the center of the slot rectangle
    //     vec3s slotPos    = Sol_Xform_GetPos(world, id);
    //     vec2s slotCenter = {slotPos.x + (body->dims.x * 0.5f), slotPos.y + (body->dims.y * 0.5f)};
    //     int   slot       = abilitySlot->slot;

    //     int   bestCardId    = -1;
    //     float minDistanceSq = 9999999.0f;

    //     int overlappingIds[24];
    //     int overlapCount = Sol_Body2d_GetOverlapping(world, id, overlappingIds, 24);

    //     for (int j = 0; j < overlapCount; j++)
    //     {
    //         int          overlappingId = overlappingIds[j];
    //         CompHudItem *hud_item      = Sol_Hud_GetItem(world, overlappingId);
    //         if (hud_item)
    //         {
    //             CompBody2d *cardBody = &body2ds[overlappingId];
    //             vec3s       cardPos  = Sol_Xform_GetPos(world, overlappingId);

    //             // Find the center of the card rectangle
    //             vec2s cardCenter = {cardPos.x + (cardBody->dims.x * 0.5f), cardPos.y + (cardBody->dims.y * 0.5f)};

    //             // Calculate squared distance (skipping sqrt for performance)
    //             float dx     = cardCenter.x - slotCenter.x;
    //             float dy     = cardCenter.y - slotCenter.y;
    //             float distSq = (dx * dx) + (dy * dy);

    //             // The closest card to the absolute center of this slot wins ownership
    //             if (distSq < minDistanceSq)
    //             {
    //                 minDistanceSq = distSq;
    //                 bestCardId    = overlappingId;
    //             }
    //         }
    //     }

    //     SolItem bestCardItem = items[bestCardId].item;
    //     // Sol_Ability_RequestBind(activeWorld, playerId, abilitySlot->slot, bestCardItem.ability,
    //     bestCardItem.rarity,
    //     //                         bestCardItem.bonusDamage, bestCardItem.bonusBuffs, bestCardItem.bonusEffects);

    //     vec3s       cardPos    = Sol_Xform_GetPos(world, bestCardId);
    //     CompBody2d *cardBody   = &body2ds[bestCardId];
    //     float       halfWidth  = cardBody->dims.x * 0.5f;
    //     float       halfHeight = cardBody->dims.y * 0.5f;
    //     vec2s       cardCenter = {cardPos.x + (halfWidth), cardPos.y + (halfHeight)};

    //     float dx          = slotCenter.x - cardCenter.x;
    //     float dy          = slotCenter.y - cardCenter.y;
    //     vec2s anchorVel   = world->body2d[Sol_Parent_GetParent(world, id)].vel;
    //     float anchorSpeed = glms_vec2_norm2(anchorVel);

    //     // cards arent parented to slots, but slots are parented to draggable section bar
    //     if (interacts[Sol_Parent_GetParent(world, id)].state & INTERACT_DRAGGING ||
    //         anchorSpeed > glms_vec2_norm2(cardBody->vel))
    //     {
    //         cardBody->vel = anchorVel;
    //     }
    //     else if (!(interacts[bestCardId].state & INTERACT_DRAGGING))
    //     {
    //         cardBody->vel.x = fmaxf(-5.0f, fminf(5.0f, dx));
    //         cardBody->vel.y = fmaxf(-5.0f, fminf(5.0f, dy));
    //     }

    //     CompAbility *ability    = Sol_Ability_Get(activeWorld, playerId);
    //     SolView2d   *cdView     = &view2ds[id].views[6];
    //     SolView2d   *activeView = &view2ds[id].views[5];
    //     SolView2d   *pressView  = &view2ds[id].views[3];
    //     SolView2d   *cdFlash    = &view2ds[id].views[7];
    //     SolView2d   *slotText   = &view2ds[id].views[4];
    //     if (!ability)
    //         continue;

    //     if (bestCardItem.ability.state != 0)
    //     {
    //         AbilityStateData *data             = &ability->stateData[slot];
    //         float             elapsed          = solState.gameTime - data->lastExited;
    //         float             cooldownDuration = 0;
    //         float             duration         = 0;

    //         if (ability->activeSlot == slot)
    //         {
    //             activeView->color = (vec4s){0.5f, 1.0f, 0.5f, 0.5f};
    //         }
    //         else
    //         {
    //             activeView->color = (vec4s){1.0f, 1.0f, 1.0f, 0.0f};
    //         }

    //         bool currentlyOnCooldown = data ? (cooldownDuration > 0.0f && elapsed < cooldownDuration) : false;

    //         if (currentlyOnCooldown)
    //         {
    //             cdView->fill = cdView->targetFill =
    //                 cooldownDuration < 0.001f ? 0.0f : 1.0f - (elapsed / cooldownDuration);
    //             abilitySlot->onCooldown = true;
    //             slotText->color         = (vec4s){1.0f, 0, 0, 1.0f};
    //         }
    //         else
    //         {
    //             cdView->fill = cdView->targetFill = 0.0f;
    //             slotText->color                   = (vec4s){0.0f, 1.0f, 0.1f, 1.0f};

    //             if (abilitySlot->onCooldown)
    //             {
    //                 cdFlash->clickAnim      = 1.0f;
    //                 abilitySlot->onCooldown = false;
    //             }
    //         }
    //     }
    //     else
    //     {
    //         cdView->fill = cdView->targetFill = 0.0f;
    //         abilitySlot->onCooldown           = false;
    //     }
    // }
}

static void Item_Tick(World *world, double dt, double time)
{
    WorldHuds *wc = world->dense_components[WORLD_SYS_HUD];
    if (!wc)
        return;
    World *game_world = Sol_GetActiveGameWorld();
    if (!game_world)
        return;
    int            player_id = Sol_Player_GetEnt(game_world, 0);
    CompInventory *inv       = (player_id >= 0) ? Sol_Inventory_Get(game_world, player_id) : NULL;
    if (!inv)
        return;

    int target_cnt = inv ? inv->cnt : 0;

    // 1. Shrink HUD entities if inventory count went down
    while (wc->cnt > target_cnt)
    {
        // Destroy the last HUD entity (slot index wc->cnt - 1)
        int last_ent = wc->dense[wc->cnt - 1];
        Sol_Destroy_Ent(world, last_ent);
    }

    // 2. Expand HUD entities if inventory count went up
    while (wc->cnt < target_cnt)
    {
        int idx = wc->cnt;
        // vec3s pos = Sol_HUD_GetSlotScreenPos(idx);
        vec3s pos = {0, 0, 0};

        // Spawn 2D HUD Card Entity corresponding directly to idx
        int          card_ent  = Sol_Prefab_AbilityCard(world, pos, inv->items[idx].ability.state, 0);
        CompHudItem *card_comp = Sol_Hud_AddItem(world, card_ent);
        *card_comp             = (CompHudItem){.idx = idx};
    }

    // 3. Simple 1:1 Refresh Loop
    // wc->dense[i] maps directly to inv->items[i]
    for (int i = 0; i < wc->cnt; i++)
    {
        int      hud_ent = wc->dense[i];
        SolItem *item    = &inv->items[i];

        // Update the visual representation of HUD entity `hud_ent` using `item->ability`
        // e.g., update sprite texture, rarity color, or text render components directly
        // Sol_HUDCard_UpdateVisuals(world, hud_ent, item);
    }
}

void Sol_Hud_Init(World *world)
{
    WorldHuds *wc                          = malloc(sizeof(WorldHuds));
    world->dense_components[WORLD_SYS_HUD] = wc;
    wc->cap                                = 32;
    wc->cnt                                = 0;
    wc->sparse                             = malloc(sizeof(int) * MAX_ENTS);
    wc->dense                              = malloc(sizeof(int) * wc->cap);
    wc->hud_items                          = malloc(sizeof(CompHudItem) * wc->cap);
    memset(wc->sparse, -1, sizeof(int) * MAX_ENTS);

    WAddTick(world) = Item_Tick;
    WAddTick(world) = Slot_Tick;
}

CompHudItem *Sol_Hud_AddItem(World *world, int id)
{
    WorldHuds *wc = world->dense_components[WORLD_SYS_HUD];
    if (wc->sparse[id] != -1)
        return &wc->hud_items[id];
    if (wc->cnt >= wc->cap)
    {
        wc->cap       = wc->cap == 0 ? 4 : wc->cap * 2;
        wc->dense     = realloc(wc->dense, sizeof(int) * wc->cap);
        wc->hud_items = realloc(wc->hud_items, sizeof(CompHudItem) * wc->cap);
    }
    int idx               = wc->cnt++;
    wc->sparse[id]        = idx;
    wc->dense[idx]        = id;
    CompHudItem *hud_item = &wc->hud_items[idx];
    *hud_item             = (CompHudItem){0};

    return hud_item;
}

CompHudItem *Sol_Hud_GetItem(World *world, int id)
{
    WorldHuds *wc = world->dense_components[WORLD_SYS_HUD];
    if (wc->sparse[id] < 0)
        return NULL;

    return &wc->hud_items[wc->sparse[id]];
}

CompAbilitySlot *Sol_Hud_AddSlot(World *world, int id)
{
    return NULL;
}

void Sol_Hud_Rem(World *world, int id)
{
    WorldHuds *wc = world->dense_components[WORLD_SYS_HUD];
    if (!wc)
        return;
    int idx = wc->sparse[id];
    if (idx < 0)
        return;

    int lastIdx        = wc->cnt - 1;
    int lastId         = wc->dense[lastIdx];
    wc->hud_items[idx] = wc->hud_items[lastIdx];
    wc->dense[idx]     = lastId;
    wc->sparse[lastId] = idx;
    wc->sparse[id]     = -1;
    wc->cnt--;
}
