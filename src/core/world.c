#include "sol_core.h"

World *World_Create(void)
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->worldActive = true;

        SolState *state                    = Sol_GetState();
        state->worlds[state->worldCount++] = world;
        world->playerID                    = -1;
    }

    return world;
}

World *World_Create_Default(void)
{
    World *world = World_Create();
    if (world)
        for (int i = 0; i < WORLD_SYS_COUNT; i++)
            World_System_Add(world, i);
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

void World_System_Add(World *world, WorldSystem system)
{
    if (!world)
        return;
    if (world_systems[system].init)
        world_systems[system].init(world);
    if (world_systems[system].step)
        world->stepSystems[world->stepCount++] = world_systems[system].step;
    if (world_systems[system].tick)
        world->tickSystems[world->tickCount++] = world_systems[system].tick;
    if (world_systems[system].draw)
        world->drawSystems[world->drawCount++] = world_systems[system].draw;
}

int Entity_Create(World *world)
{
    for (int i = 0; i < MAX_ENTS; i++)
    {
        if (!world->actives[i])
        {
            world->actives[i] = true;
            world->masks[i]   = HAS_NONE;

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
    world->masks[id]   = 0;

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

CompShape *Entity_Add_Shape(World *world, int id)
{
    world->masks[id] |= HAS_SHAPE;
    return &world->shapes[id];
}

CompInteractable *Entity_Add_Interact(World *world, int id)
{

    world->masks[id] |= HAS_INTERACT;
    return &world->interactables[id];
}

CompInfo *Entity_Add_Info(World *world, int id)
{
    world->masks[id] |= HAS_INFO;
    return &world->infos[id];
}

CompUiElement *Entity_Add_UiElement(World *world, int id)
{
    world->masks[id] |= HAS_UI_ELEMENT;
    return &world->uiElements[id];
}

CompMovement *Entity_Add_Movement(World *world, int id)
{
    world->masks[id] |= HAS_MOVEMENT;
    return &world->movements[id];
}

CompController *Entity_Add_Controller_Local(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER;
    world->playerID = id;
    return &world->controllers[id];
}

CompController *Entity_Add_Controller_Remote(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER;
    return &world->controllers[id];
}

CompController *Entity_Add_Controller_Ai(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER_AI;
    return &world->controllers[id];
}

CompModel *Entity_Add_Model(World *world, int id, SolModelId model)
{
    world->models[id] = (CompModel){
        .gpuHandle = model,
        .model     = Sol_GetModel(model),
    };
    world->masks[id] |= HAS_MODEL;
    return &world->models[id];
}

int Sol_World_GetEntCount(World *world)
{
    return world->activeCount;
}
