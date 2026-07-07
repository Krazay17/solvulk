/*
 * File: s_item.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-28
 * Example of how systems could operate under void *components
 */

#include "s_item.h"
#include "sol_engine.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "xform/s_xform.h"
#include "physx/s_body2d.h"
#include "game/prefabs.h"
#include "ability/s_ability.h"
#include "parent/s_parent.h"
#include "interact/s_interact.h"
#include "view/s_view2d.h"

#define MAX_ITEMS (1 << 12)

static void AbilitySlots(World *world, double dt, double time);
static void Inventory_Update(World *world, double dt, double time);
static void Inventory_Draw(World *world, double dt, double time);
static void Inventory_Deinit(World *world);
static void Item_Deinit(World *world);
static void AbilitySlot_Deinit(World *world);

void Sol_Item_Init(World *world)
{
    // CompInventory *inventory         = calloc(MAX_ENTS, sizeof(CompInventory));
    // world->components[HAS_INVENTORY] = inventory;
    world->inventories = calloc(MAX_ENTS, sizeof(CompInventory));

    // CompItem *item              = calloc(MAX_ENTS, sizeof(CompItem));
    // world->components[HAS_ITEM] = item;
    world->items = calloc(MAX_ENTS, sizeof(CompItem));

    // CompAbilitySlot *abilitySlot       = calloc(MAX_ENTS, sizeof(CompAbilitySlot));
    // world->components[HAS_ABILITYSLOT] = abilitySlot;
    world->abilitySlots = calloc(MAX_ENTS, sizeof(CompAbilitySlot));

    WAddStep(world)   = AbilitySlots;
    WAddStep(world)   = Inventory_Update;
    WAdd2d(world)     = Inventory_Draw;
    WAddDeinit(world) = Inventory_Deinit;
    WAddDeinit(world) = Item_Deinit;
    WAddDeinit(world) = AbilitySlot_Deinit;
}

void Inventory_Deinit(World *world)
{
}
void Item_Deinit(World *world)
{
}
void AbilitySlot_Deinit(World *world)
{
}

void Sol_Item_AddAbility(World *world, int id, u32 ability)
{
    CompItem new     = {0};
    new.item.ability = ability;
    CompItem *item   = &world->items[id];
    *item            = new;
    world->masks[id] |= BITC(HAS_ITEM);
}

void Sol_Inventory_AddItem(World *world, int id, Item item)
{
    CompInventory *inventory = &world->inventories[id];
    if (Sol_Realloc(&inventory->items, inventory->cnt, &inventory->cap, sizeof(Item)) != 0)
        return;
    inventory->items[inventory->cnt++] = item;
}

void Sol_Item_AddAbilitySlot(World *world, int id, int slot)
{
    world->abilitySlots[id].slot = slot;
    world->masks[id] |= BITC(HAS_ABILITYSLOT);
}

static int ability_slots_required = BITC(HAS_ABILITYSLOT);
static void AbilitySlots(World *world, double dt, double time)
{
    CompItem        *items        = world->items;
    CompAbilitySlot *abilitySlots = world->abilitySlots;
    CompBody2d      *body2ds      = world->body2d;
    CompInteract    *interacts    = world->interacts;
    CompView2d      *view2ds      = world->view2d;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, ability_slots_required))
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

        World *activeWorld = solEngine.activeWorld;
        if (bestCardId == -1)
        {
            Sol_Ability_RequestBind(activeWorld, 1, abilitySlot->slot, 0, 0, 0, 0, 0);
            continue;
        }
        Item bestCardItem = items[bestCardId].item;
        Sol_Ability_RequestBind(activeWorld, 1, abilitySlot->slot, bestCardItem.ability, bestCardItem.rarity,
                                bestCardItem.bonusDamage, bestCardItem.bonusBuffs, bestCardItem.bonusEffects);

        vec3s       cardPos    = Sol_Xform_GetPos(world, bestCardId);
        CompBody2d *cardBody   = &body2ds[bestCardId];
        float       halfWidth  = cardBody->dims.x * 0.5f;
        float       halfHeight = cardBody->dims.y * 0.5f;
        vec2s       cardCenter = {cardPos.x + (halfWidth), cardPos.y + (halfHeight)};

        float dx = slotCenter.x - cardCenter.x;
        float dy = slotCenter.y - cardCenter.y;

        cardBody->vel.x = fmaxf(-5.0f, fminf(5.0f, dx));
        cardBody->vel.y = fmaxf(-5.0f, fminf(5.0f, dy));

        if (interacts[Sol_Parent_GetParent(world, id)].state & INTERACT_MOVING)
        {
            Sol_Xform_SetPos(world, bestCardId, (vec3s){slotCenter.x - halfWidth, slotCenter.y - halfHeight, 0});
        }

        CompAbility *ability    = &activeWorld->abilities[1];
        SolView2d   *cdView     = &view2ds[id].views[6];
        SolView2d   *activeView = &view2ds[id].views[5];
        SolView2d   *pressView  = &view2ds[id].views[3];
        SolView2d   *cdFlash    = &view2ds[id].views[7];
        SolView2d   *slotText   = &view2ds[id].views[4];

        if (bestCardItem.ability != 0)
        {
            AbilityData *data             = &ability->stateData[slot];
            float        elapsed          = solState.gameTime - data->lastExited;
            float        cooldownDuration = data->cooldown;
            float        duration         = data->duration;

            if (Sol_Parent_IsActive(world, bestCardId))
            {
                CompBody2d *cardBody  = &body2ds[bestCardId];
                vec2s       offsetPos = glms_vec2_sub(body->dims, cardBody->dims);
                offsetPos             = glms_vec2_scale(offsetPos, 0.5f);
                offsetPos             = glms_vec2_add(offsetPos, (vec2s){slotPos.x, slotPos.y});

                Sol_Xform_SetPos(world, bestCardId, (vec3s){offsetPos.x, offsetPos.y});
            }

            if (ability->activeSlot == slot)
            {
                activeView->color = (vec4s){0.5f, 1.0f, 0.5f, 0.5f};
            }
            else
            {
                activeView->color = (vec4s){1.0f, 1.0f, 1.0f, 0.0f};
            }

            bool currentlyOnCooldown = (cooldownDuration > 0.0f && elapsed < cooldownDuration);

            if (currentlyOnCooldown)
            {
                cdView->fill = cdView->targetFill = 1.0f - (elapsed / cooldownDuration);
                abilitySlot->onCooldown           = true;
                slotText->color                   = (vec4s){1.0f, 0, 0, 1.0f};
            }
            else
            {
                cdView->fill = cdView->targetFill = 0.0f;
                slotText->color                   = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};

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

        if ((Sol_Controller_GetActionState(activeWorld, 1) & ability->bindings[slot].actionBit) != 0)
        {
            pressView->color = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
        }
        else
        {
            pressView->color = (vec4s){1.0f, 1.0f, 1.0f, 0.0f};
        }
    }
}

struct Query
{
    CompInventory   *inv;
    CompItem        *item;
    CompAbilitySlot *ability;
};
static void Inventory_Update(World *world, double dt, double time)
{
    // int          required = BITC(HAS_INVENTORY);
    // struct Query q        = {
    //     .inv     = world->components[HAS_INVENTORY],
    //     .item    = world->components[HAS_ITEM],
    //     .ability = world->components[HAS_ABILITYSLOT],
    // };

    // for (int i = 0; i < world->activeCount; i++)
    // {
    //     int id = world->activeEntities[i];
    //     if ((world->masks[id] & required) != required)
    //         continue;
    //     CompAbilitySlot *slot = &q.ability[id];
    // }
}

static int inventory_draw_required = BITC(HAS_INVENTORY);
static void Inventory_Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, inventory_draw_required))
            continue;
    }
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