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

void Sol_Item_AddAbility(World *world, int id, AbilityState ability)
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
        CompBody2d *body = &world->body2d[id];

        int count = 0;
        for (int j = 0; j < body->overlapCount; j++)
        {
            int overlappingId = body->overlapping[j];
            if (world->masks[overlappingId] & HAS_ITEM)
            {
                count++;
                CompItem *loot = &world->items[overlappingId];
                Sol_Ability_SetAbility(Sol_GetState()->activeWorld, 1, item->slot, loot->ability);
            }
        }
        if (count == 0)
            Sol_Ability_SetAbility(Sol_GetState()->activeWorld, 1, item->slot, 0);
    }
}
