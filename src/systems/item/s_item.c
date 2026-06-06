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
    CompItem item    = {.ability = ability, .kind = ITEMKIND_ABILITY_CARD};
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
        CompAbility *ability    = &activeWorld->abilities[1];
        SolView2d   *cdView     = &world->view2d[id].views[6];
        SolView2d   *activeView = &world->view2d[id].views[5];
        SolView2d   *pressView  = &world->view2d[id].views[3];
        SolView2d   *cdFlash    = &world->view2d[id].views[7];
        SolView2d   *slotText   = &world->view2d[id].views[4];

        if (bestAbility != 0)
        {
            CompAbility *ability          = &activeWorld->abilities[1];
            AbilityData *data             = &ability->stateData[item->slot];
            float        elapsed          = Sol_GetGameTime() - data->lastExited;
            float        cooldownDuration = ability_config[bestAbility].cooldown;
            float        duration         = ability_config[bestAbility].duration;

            if (Sol_Parent_IsActive(world, bestCardId))
            {
                CompBody2d *cardBody  = &world->body2d[bestCardId];
                vec2s       offsetPos = glms_vec2_sub(body->dims, cardBody->dims);
                offsetPos             = glms_vec2_scale(offsetPos, 0.5f);
                offsetPos             = glms_vec2_add(offsetPos, (vec2s){slotPos.x, slotPos.y});

                Sol_Xform_SetPos(world, bestCardId, (vec3s){offsetPos.x, offsetPos.y});
            }

            if (ability->activeSlot == item->slot)
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
                item->onCooldown                  = true;
                slotText->color                   = (vec4s){1.0f, 0, 0, 1.0f};
            }
            else
            {
                cdView->fill = cdView->targetFill = 0.0f;
                slotText->color                   = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};

                if (item->onCooldown)
                {
                    cdFlash->clickAnim = 1.0f;
                    item->onCooldown   = false;
                }
            }
        }
        else
        {
            cdView->fill = cdView->targetFill = 0.0f;
            item->onCooldown                  = false;
        }

        if ((Sol_Controller_GetActionState(activeWorld, 1) & ability->bindings[item->slot].actionBit) != 0)
        {
            pressView->color = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
        }
        else
        {
            pressView->color = (vec4s){1.0f, 1.0f, 1.0f, 0.0f};
        }
    }
}

void Sol_Item_SetRarity(World *world, int id, u32 rarity)
{
    world->items[id].rarity = rarity;
    vec4s color;
    switch (rarity)
    {
    case 0:
        color = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
        break;
    case 1:
        color = (vec4s){0.0f, 1.0f, 0.0f, 1.0f};
        break;
    case 2:
        color = (vec4s){1.0f, 0.0f, 1.0f, 1.0f};
        break;
    case 3:
        color = (vec4s){0.0f, 1.0f, 1.0f, 1.0f};
        break;
    }
    world->view2d[id].views[1].color = color;
    world->view2d[id].views[1].hoverColor = color;
}