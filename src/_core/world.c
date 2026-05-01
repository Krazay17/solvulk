#include "sol_core.h"

static SystemConfig world_systems[WORLD_SYS_COUNT] = {
    [WORLD_SYS_TIMER] = {.tick = Timer_Tick},
    [WORLD_SYS_PHYSX] = {.init = Physx_Init, .step = Physx_Step},

    [WORLD_SYS_CONTROLLER_LOCAL] = {.tick = Sol_System_Controller_Local_Tick},
    [WORLD_SYS_CONTROLLER_AI]    = {.tick = Sol_System_Controller_Ai_Tick},
    [WORLD_SYS_INTERACT]         = {.tick = Interact2d_Tick},
    [WORLD_SYS_MOVEMENT]         = {.step = Sol_System_Movement_3d_Step},
    [WORLD_SYS_COMBAT]           = {.tick = Combat_Tick},
    [WORLD_SYS_BUFF]             = {.step = Buff_Step},
    [WORLD_SYS_VITAL]            = {.step = Vital_Step, .draw = Vital_Draw},
    [WORLD_SYS_MODEL]            = {.draw = Sol_System_Model_Draw},
    [WORLD_SYS_UI]               = {.draw = UiView_Draw},
    [WORLD_SYS_LINE]             = {.init = Lines_Init, .tick = Sol_System_Line_Tick, .draw = Sol_System_Line_Draw},
    [WORLD_SYS_EMITTER]          = {.init = Emitter_Init, .tick = Emitter_Tick, .draw = Emitter_Draw},
    [WORLD_SYS_SPHERE]           = {.draw = Sphere_Draw},
    [WORLD_SYS_CAM]              = {.draw = Crosshair_Draw},
    [WORLD_SYS_PICKUP]           = {.step = Pickup_Step},
};

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
    Flush_Queue();
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

int Sol_Create_Ent(World *world)
{
    for (int i = 1; i < MAX_ENTS; i++)
    {
        if (!world->actives[i])
        {
            world->actives[i] = true;
            world->masks[i]   = HAS_NONE;

            // NEW: Track this ID in our dense list
            world->activeEntities[world->activeCount] = i;
            world->activeCount++;

            Sol_Debug_Add("Entities", world->activeCount);
            return i;
        }
    }
    return -1;
}

void Sol_Destroy_Ent(World *world, int id)
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

void Sol_Flags_Add(World *world, int id, EntFlags flags)
{
    world->flags[id].flags |= flags;
}

void Sol_Flags_Remove(World *world, int id, EntFlags flags)
{
    world->flags[id].flags &= ~flags;
}

CompShape *Sol_Shape_Add(World *world, int id)
{
    world->masks[id] |= HAS_SHAPE;
    return &world->shapes[id];
}

CompInfo *Sol_Info_Add(World *world, int id)
{
    world->masks[id] |= HAS_INFO;
    return &world->infos[id];
}

CompController *Sol_ControllerRemote_Add(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER;
    return &world->controllers[id];
}

CompController *Sol_ControllerAi_Add(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER_AI;
    return &world->controllers[id];
}

int Sol_World_GetEntCount(World *world)
{
    return world->activeCount;
}
