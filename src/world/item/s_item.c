/*
 * File: s_item.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-28
 * Example of how systems could operate under void *components
 */

#include "s_item.h"
#include "sol_core.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "xform/s_xform.h"
#include "physx/s_body2d.h"
#include "game/prefabs.h"

#include "player/s_player.h"
#include "controller/s_controller.h"
#include "ability/s_ability.h"
#include "parent/s_parent.h"
#include "interact/s_interact.h"
#include "view/s_view2d.h"
#include "render/render.h"

u32 ability_icon[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_CLAW] = SOL_TEXTURE_BLADE_CARD,        [ABILITY_STATE_DASH] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_FIREBALL] = SOL_TEXTURE_FIREBALL_CARD, [ABILITY_STATE_PISTOL] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_SPINSLASH] = SOL_TEXTURE_BLADE_CARD,   [ABILITY_STATE_CLAW] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_SHIELD] = SOL_TEXTURE_BLADE_CARD,      [ABILITY_STATE_LASER] = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_WHIP] = SOL_TEXTURE_BLADE_CARD,        [ABILITY_STATE_FIREBALLVOLLEY] = SOL_TEXTURE_BLADE_CARD,
};

static void AbilitySlots(World *world, double dt, double time)
{
    return;
    static int required =
        BITC(HAS_ACTIVE) | BITC(HAS_ABILITYSLOT) | BITC(HAS_BODY2) | BITC(HAS_INTERACT) | BITC(HAS_VIEW2D);
    CompItem        *items        = world->items;
    CompAbilitySlot *abilitySlots = world->abilitySlots;
    CompBody2d      *body2ds      = world->body2d;
    CompInteract    *interacts    = world->interacts;
    CompView2d      *view2ds      = world->view2d;
    World           *activeWorld  = Sol_GetActiveGameWorld();
    int              playerId     = Sol_Player_GetEnt(activeWorld, 0);
    if (playerId < 0)
        return;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;

        CompAbilitySlot *abilitySlot = &abilitySlots[id];
        CompBody2d      *body        = &body2ds[id];

        // Find the center of the slot rectangle
        vec3s slotPos    = Sol_Xform_GetPos(world, id);
        vec2s slotCenter = {slotPos.x + (body->dims.x * 0.5f), slotPos.y + (body->dims.y * 0.5f)};
        int   slot       = abilitySlot->slot;

        int   bestCardId    = -1;
        float minDistanceSq = 9999999.0f;
        for (int j = 0; j < body->overlapCount; j++)
        {
            int overlappingId = body->overlapping[j];
            if (world->masks[overlappingId] & BITC(HAS_ITEM))
            {
                CompItem   *cardItem = &items[overlappingId];
                CompBody2d *cardBody = &body2ds[overlappingId];
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
                    bestCardId    = overlappingId;
                }
            }
        }

        if (bestCardId == -1)
        {
            Sol_Ability_RequestBind(activeWorld, playerId, abilitySlot->slot, 0, 0, 0, 0, 0);
            continue;
        }
        SolItem bestCardItem = items[bestCardId].item;
        // Sol_Ability_RequestBind(activeWorld, playerId, abilitySlot->slot, bestCardItem.ability, bestCardItem.rarity,
        //                         bestCardItem.bonusDamage, bestCardItem.bonusBuffs, bestCardItem.bonusEffects);

        vec3s       cardPos    = Sol_Xform_GetPos(world, bestCardId);
        CompBody2d *cardBody   = &body2ds[bestCardId];
        float       halfWidth  = cardBody->dims.x * 0.5f;
        float       halfHeight = cardBody->dims.y * 0.5f;
        vec2s       cardCenter = {cardPos.x + (halfWidth), cardPos.y + (halfHeight)};

        float dx          = slotCenter.x - cardCenter.x;
        float dy          = slotCenter.y - cardCenter.y;
        vec2s anchorVel   = world->body2d[Sol_Parent_GetParent(world, id)].vel;
        float anchorSpeed = glms_vec2_norm2(anchorVel);

        // cards arent parented to slots, but slots are parented to draggable section bar
        if (interacts[Sol_Parent_GetParent(world, id)].state & INTERACT_DRAGGING ||
            anchorSpeed > glms_vec2_norm2(cardBody->vel))
        {
            cardBody->vel = anchorVel;
        }
        else if (!(interacts[bestCardId].state & INTERACT_DRAGGING))
        {
            cardBody->vel.x = fmaxf(-5.0f, fminf(5.0f, dx));
            cardBody->vel.y = fmaxf(-5.0f, fminf(5.0f, dy));
        }

        CompAbility *ability    = Sol_Ability_Get(activeWorld, playerId);
        SolView2d   *cdView     = &view2ds[id].views[6];
        SolView2d   *activeView = &view2ds[id].views[5];
        SolView2d   *pressView  = &view2ds[id].views[3];
        SolView2d   *cdFlash    = &view2ds[id].views[7];
        SolView2d   *slotText   = &view2ds[id].views[4];
        if (!ability)
            continue;

        if (bestCardItem.abilityState != 0)
        {
            AbilityStateData *data             = &ability->stateData[slot];
            float             elapsed          = solState.gameTime - data->lastExited;
            float             cooldownDuration = 0;
            float             duration         = 0;

            if (ability->activeSlot == slot)
            {
                activeView->color = (vec4s){0.5f, 1.0f, 0.5f, 0.5f};
            }
            else
            {
                activeView->color = (vec4s){1.0f, 1.0f, 1.0f, 0.0f};
            }

            bool currentlyOnCooldown = data ? (cooldownDuration > 0.0f && elapsed < cooldownDuration) : false;

            if (currentlyOnCooldown)
            {
                cdView->fill = cdView->targetFill =
                    cooldownDuration < 0.001f ? 0.0f : 1.0f - (elapsed / cooldownDuration);
                abilitySlot->onCooldown = true;
                slotText->color         = (vec4s){1.0f, 0, 0, 1.0f};
            }
            else
            {
                cdView->fill = cdView->targetFill = 0.0f;
                slotText->color                   = (vec4s){0.0f, 1.0f, 0.1f, 1.0f};

                if (abilitySlot->onCooldown)
                {
                    cdFlash->clickAnim      = 1.0f;
                    abilitySlot->onCooldown = false;
                }
            }
        }
        else
        {
            cdView->fill = cdView->targetFill = 0.0f;
            abilitySlot->onCooldown           = false;
        }
    }
}

static void Draw2d(World *world, double dt, double time)
{
    static int required = BITC(HAS_BODY2) | BITC(HAS_ITEM);
    int        counter  = 0;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;
        CompItem   *item  = &world->items[id];
        CompXform  *xform = &world->xforms[id];
        CompBody2d *body  = &world->body2d[id];

        RectSSBO *rect  = Sol_Render_GetNext_Rect();
        rect->rect      = (vec4s){xform->drawPos.x, xform->drawPos.y, body->dims.x, body->dims.y};
        rect->scale     = 1.0f;
        rect->fill      = 1.0f;
        rect->color     = (vec4s){1, 1, 1, 1};
        rect->uv        = (vec4s){0, 0, 1, 1};
        rect->textureID = ability_icon[item->item.abilityState];
        counter++;
    }
}

void Sol_Item_Init(World *world)
{
    world->items        = calloc(MAX_ENTS, sizeof(CompItem));
    world->abilitySlots = calloc(MAX_ENTS, sizeof(CompAbilitySlot));

    // WAddStep(world) = AbilitySlots;
    WAdd2d(world) = Draw2d;
}

void Sol_Item_SetRarity(World *world, int id, u32 rarity)
{
    rarity               = min(3, rarity);
    CompItem    *item    = &world->items[id];
    CompTooltip *tooltip = &world->tooltips[id];
    CompView2d  *view    = &world->view2d[id];
    item->item.rarity    = rarity;
    vec4s color;
    switch (rarity)
    {
    case 0:
        color = (vec4s){0.5f, 0.5f, 0.5f, 1.0f};
        break;
    case 1:
        color = (vec4s){0.0f, 1.0f, 0.0f, 1.0f};
        break;
    case 2:
        color = (vec4s){1.0f, 0.0f, 1.0f, 1.0f};
        break;
    case 3:
        color = (vec4s){1.0f, 0.2f, 0.0f, 1.0f};
        break;
    }
    view->views[1].color      = color;
    view->views[1].hoverColor = color;
}

void Sol_Item_Drop(World *world, int id)
{
    // Sol_Prefab_ItemDrop(world, Sol_Xform_GetPos(world, id));
}