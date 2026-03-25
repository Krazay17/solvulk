#include <assert.h>

#include "world.h"
#include "stdlib.h"

World *World_Create(void)
{
    World *world = calloc(1, sizeof(World));
    if (world)
        world->worldActive = true;
    return world;
}

void World_Destroy(World *world)
{
    if (world)
        free(world);
}

void World_Step(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->stepCount; i++)
    {
        world->stepSystems[i](world, dt, time);
    }
}

void World_Tick(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->tickCount; i++)
    {
        world->tickSystems[i](world, dt, time);
    }
}

void World_Draw(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->drawCount; i++)
    {
        world->drawSystems[i](world, dt, time);
    }
}

void World_System_Add(World *world, SystemFunc func, SystemType type)
{
    if (!world || !func)
        return;

    switch (type)
    {
    case SYSTEM_STEP:
        if (world->stepCount < MAX_SYSTEMS)
            world->stepSystems[world->stepCount++] = func;
        break;
    case SYSTEM_TICK:
        if (world->tickCount < MAX_SYSTEMS)
            world->tickSystems[world->tickCount++] = func;
        break;
    case SYSTEM_DRAW:
        if (world->drawCount < MAX_SYSTEMS)
            world->drawSystems[world->drawCount++] = func;
        break;
    }
}

int Entity_Create(World *world)
{
    for (int i = 0; i < MAX_ENTS; i++)
    {
        if (!world->actives[i])
        {
            world->actives[i] = true;
            world->masks[i] = HAS_NONE;

            // NEW: Track this ID in our dense list
            world->activeEntities[world->activeCount] = i;
            world->activeCount++;

            return i;
        }
    }
    return -1;
}

void Entity_Destroy(World *world, int id)
{
    // 1. Mark as inactive
    world->actives[id] = false;
    world->masks[id] = 0;

    // 2. Remove from dense list by "Swapping with Last"
    // This keeps the array dense without needing to shift everything
    for (int i = 0; i < world->activeCount; i++)
    {
        if (world->activeEntities[i] == id)
        {
            // Take the very last ID in the list and put it here
            world->activeEntities[i] = world->activeEntities[world->activeCount - 1];
            world->activeCount--;
            break;
        }
    }
}

void Entity_Add_Xform(World *world, int id, Xform xform)
{
    world->xforms[id] = xform;
    world->masks[id] |= HAS_XFORM;
}

void Entity_Add_Veloc(World *world, int id, Veloc veloc)
{
    world->velocs[id] = veloc;
    world->masks[id] |= HAS_BODY;
}

void Entity_Add_Rect(World *world, int id, CompRect rect)
{
    world->rects[id] = rect;
    world->masks[id] |= HAS_BUTTON;
}

