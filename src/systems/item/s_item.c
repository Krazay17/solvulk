#include "sol_core.h"

#define SLOTCOUNT 10

static void AbilitySlots(World *world, double dt, double time);

void Sol_Item_Init(World *world)
{
    world->items    = calloc(MAX_ENTS, sizeof(CompItem));
    WAddStep(world) = AbilitySlots;
}

CompItem *Sol_Item_Add(World *world, int id, ItemKind kind)
{
    CompItem item    = {.kind = kind};
    world->items[id] = item;
    world->masks[id] |= HAS_ITEM;
    return &world->items[id];
}

void Sol_Item_AddAbility(World *world, int id, u32 ability)
{
    CompItem item = {.ability = ability};
    // item.kind        = ITEMKIND_ABILITY_CARD;
    world->items[id] = item;
    world->masks[id] |= HAS_ITEM;
}

void Sol_Item_AddAbilitySlot(World *world, int id, int slot)
{
    CompItem item    = {.slot = slot, .kind = ITEMKIND_ABILITY_SLOT};
    world->items[id] = item;
    world->masks[id] |= HAS_ITEM;
}

static void AbilitySlots(World *world, double dt, double time)
{
    int required = HAS_ITEM;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompItem *item = &world->items[id];
        if (item->kind != ITEMKIND_ABILITY_SLOT)
            continue;
        CompBody2d *body    = &world->body2d[id];
        vec3s       slotPos = Sol_Xform_GetPos(world, id);

        // Find the center of the slot rectangle
        vec2s slotCenter = {slotPos.x + (body->dims.x * 0.5f), slotPos.y + (body->dims.y * 0.5f)};

        int          count         = 0;
        int          bestCardId    = -1;
        AbilityState bestAbility   = 0;
        float        minDistanceSq = 9999999.0f;
        for (int j = 0; j < body->overlapCount; j++)
        {
            int overlappingId = body->overlapping[j];
            if (world->masks[overlappingId] & HAS_ITEM)
            {
                count++;
                CompItem *cardItem = &world->items[overlappingId];
                // If you add other item types later, ensure we only track cards here

                CompBody2d *cardBody = &world->body2d[overlappingId];
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
                    bestAbility   = cardItem->ability;
                }
            }
        }
        World *activeWorld = Sol_GetState()->activeWorld;
        if (bestCardId != -1)
        {
            Sol_Ability_SetAbility(activeWorld, 1, item->slot, bestAbility);
        }
        else
        {
            Sol_Ability_SetAbility(activeWorld, 1, item->slot, 0);
        }
        CompAbility *ability = &activeWorld->abilities[1];
        SolView2d   *cdView  = &world->view2d[id].views[3];

        if (bestAbility != 0)
        {
            CompAbility *ability  = &activeWorld->abilities[1];
            AbilityData *data     = &ability->stateData[item->slot];
            float        elapsed  = Sol_GetGameTime() - data->lastExited;
            float        duration = ability_config[bestAbility].cooldown;

            if (duration > 0.0f && elapsed < duration)
            {
                // 1.0 right after use, drains to 0 as ability becomes ready
                cdView->targetFill = 1.0f - (elapsed / duration);
            }
            else
            {
                cdView->targetFill = 0.0f;
            }
        }
        else
        {
            cdView->targetFill = 0.0f; // no card → no cooldown
        }
    }
}
