/*
 * File: world.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * World!
 */

#include "sol_core.h"
#include "world.h"

typedef enum
{
    UPDATEPHASE_TICK,
    UPDATEPHASE_STEP,
    UPDATEPHASE_POSTTICK,
    UPDATEPHASE_RENDER3,
    UPDATEPHASE_RENDER2,
} UpdatePhase;

typedef struct
{
    SystemUpdate update;
    UpdatePhase  phase;
} SystemUpdateDef;
#define SYSTEMUPDATEDEF_COUNT 3
typedef struct
{
    SystemInit      init;
    SystemDeinit    deinit;
    SystemUpdateDef update[SYSTEMUPDATEDEF_COUNT];
} SystemDef;

const SystemDef system_inits[WORLDSYS_COUNT] = {
    [WORLDSYS_CONTROLLER] = {.update = {Controller_Tick, UPDATEPHASE_TICK}},
    [WORLDSYS_MOVE3]      = {.update = {Move3_Step, UPDATEPHASE_STEP}},
    [WORLDSYS_PHYSX]      = {.init = Physx_Init, .update = {Physx_Step, UPDATEPHASE_STEP}},
    [WORLDSYS_CAMERA]     = {.update = {Camera_Tick, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_ANIM]       = {.update = {Anim_Tick, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_MODEL]      = {.update = {Model_Render, UPDATEPHASE_RENDER3}},
    [WORLDSYS_DEBUG] =
        {
            .init      = Debug_Init,
            .update[0] = {Debug_Tick, UPDATEPHASE_TICK},
            .update[1] = {Debug_Draw3, UPDATEPHASE_RENDER3},
            .update[2] = {Debug_Draw2, UPDATEPHASE_RENDER2},
        },
};

World *World_Create()
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->maxEntities                     = MAX_ENTS;
        world->doesSimulate                    = true;
        world->doesRender                      = true;
        solState.worlds[solState.worldCount++] = world;
        Sol_World_InitAllComponents(world, world->maxEntities);
    }

    return world;
}

void World_Destroy(World *world)
{
    if (world)
    {
        for (int i = 0; i < WORLDSYS_COUNT; i++)
        {
            if (world->system_mask & BITC(i))
                Sol_Sys_Remove(world, i);
        }
        Sol_World_FreeAllComponents(world);

        // Swap-with-back removal to keep solState.worlds contiguous
        for (int i = 0; i < solState.worldCount; i++)
        {
            if (solState.worlds[i] == world)
            {
                solState.worlds[i]                   = solState.worlds[--solState.worldCount];
                solState.worlds[solState.worldCount] = NULL;
                break;
            }
        }

        free(world);
    }
}

void Sol_Sys_Add(World *world, WorldSystems system)
{
    if (world->system_mask & BITC(system))
        return;
    if (system_inits[system].init)
        system_inits[system].init(world);
    if (system_inits[system].update)
        for (int i = 0; i < SYSTEMUPDATEDEF_COUNT; i++)
        {
            if (!system_inits[system].update[i].update)
                continue;
            switch (system_inits[system].update[i].phase)
            {
            case UPDATEPHASE_TICK:
                WAddTick(world) = system_inits[system].update[i].update;
                break;
            case UPDATEPHASE_STEP:
                WAddStep(world) = system_inits[system].update[i].update;
                break;
            case UPDATEPHASE_POSTTICK:
                WAddPosttick(world) = system_inits[system].update[i].update;
                break;
            case UPDATEPHASE_RENDER3:
                WAdd3d(world) = system_inits[system].update[i].update;
                break;
            case UPDATEPHASE_RENDER2:
                WAdd2d(world) = system_inits[system].update[i].update;
                break;
            }
        }
    world->system_mask |= BITC(system);
}

static void RemoveSystemFromList(SystemUpdate *list, int *count, SystemUpdate sys)
{
    for (int i = 0; i < *count; i++)
    {
        if (list[i] == sys)
        {
            // Swap last element into this slot to keep array contiguous
            list[i]          = list[*count - 1];
            list[*count - 1] = NULL;
            (*count)--;
            return;
        }
    }
}

void Sol_Sys_Remove(World *world, WorldSystems system)
{
    if (!(world->system_mask & BITC(system)))
        return;

    if (system_inits[system].deinit)
        system_inits[system].deinit(world);

    for (int i = 0; i < SYSTEMUPDATEDEF_COUNT; i++)
    {
        SystemUpdate update_fn = system_inits[system].update[i].update;
        if (update_fn)
        {
            switch (system_inits[system].update[i].phase)
            {
            case UPDATEPHASE_TICK:
                RemoveSystemFromList(world->tickSystems, &world->tickCount, update_fn);
                break;
            case UPDATEPHASE_STEP:
                RemoveSystemFromList(world->stepSystems, &world->stepCount, update_fn);
                break;
            case UPDATEPHASE_POSTTICK:
                RemoveSystemFromList(world->posttickSystems, &world->posttickCount, update_fn);
                break;
            case UPDATEPHASE_RENDER3:
                RemoveSystemFromList(world->draw3dSystems, &world->draw3dCount, update_fn);
                break;
            case UPDATEPHASE_RENDER2:
                RemoveSystemFromList(world->draw2dSystems, &world->draw2dCount, update_fn);
                break;
            }
        }
    }

    world->system_mask &= ~BITC(system);
}

void Worlds_Step(World **worlds, int count, double dt)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;
        world->currentStep++;
        world->stepTime += dt;
        for (int i = 0; i < world->stepCount; i++)
        {
            world->stepSystems[i](world, dt);
        }
    }
}

void Worlds_Tick(World **worlds, int worldCount, double dt)
{
    for (int i = 0; i < worldCount; i++)
    {
        World *world = worlds[i];
        if (world->doesSimulate)
        {
            for (int w = 0; w < world->tickCount; w++)
                world->tickSystems[w](world, dt);
            world->currentTick++;
            world->tickTime += dt;
        }
    }
}

void Worlds_PostTick(World **worlds, int count, double dt)
{
    for (int i = 0; i < count; i++)
    {
        World *world = worlds[i];
        if (world->doesSimulate)
        {
            for (int w = 0; w < world->posttickCount; w++)
                world->posttickSystems[w](world, dt);
        }
    }
}

void Worlds_Draw3d(World **worlds, int count, double dt)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world->doesRender)
            for (int i = 0; i < world->draw3dCount; i++)
                world->draw3dSystems[i](world, dt);
    }
}

void Worlds_Draw2d(World **worlds, int count, double dt)
{
    for (int w = count - 1; w >= 0; w--)
    {
        World *world = worlds[w];
        if (world->doesRender)
            for (int i = 0; i < world->draw2dCount; i++)
                world->draw2dSystems[i](world, dt);
    }
}

int Sol_Create_Ent(World *world)
{
    int id = 0;
    while (id < world->maxEntities && world->masks[id] != 0)
        id++;

    if (id >= world->maxEntities)
        return -1;

    world->activeEnts[id] = true;
    world->entCount++;
    ScActive *sol_active       = Sol_Comp_Add(world, id, ScActive);
    sol_active->active_at_tick = world->currentTick;
    sol_active->time_activated = world->tickTime;
    Sol_Debug_Add("Entities", world->entCount);

    return id;
}
