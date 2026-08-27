/*
 * File: world.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * World!
 */

#include "sol_core.h"
#include "sol_math.h"
#include "world.h"

World *World_Create()
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->maxEntities                     = MAX_ENTITIES;
        world->doesSimulate                    = true;
        world->doesRender                      = true;
        solState.worlds[solState.worldCount++] = world;
        Sol_World_InitAllComponents(world, world->maxEntities);
    }

    return world;
}

void Sol_System_Remove_Noop(World *world, int id)
{
}

void World_Destroy(World *world)
{
    if (world)
    {
        free(world);
    }
}

void Worlds_Step(World **worlds, int count, double dt, double time)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;
        for (int i = 0; i < world->prestepCount; i++)
        {
            world->prestepSystems[i](world, dt, time);
        }
        for (int i = 0; i < world->stepCount; i++)
        {
            world->stepSystems[i](world, dt, time);
        }
        for (int i = 0; i < world->poststepCount; i++)
        {
            world->poststepSystems[i](world, dt, time);
        }
        world->currentTick++;
    }
}

void Worlds_Tick(World **worlds, int worldCount, double dt, double time)
{
    for (int i = 0; i < worldCount; i++)
    {
        World *world = worlds[i];
        if (world->doesSimulate)
        {
            for (int w = 0; w < world->tickCount; w++)
                world->tickSystems[w](world, dt, time);
            world->currentTick++;
        }
    }
}

void Worlds_Draw3d(World **worlds, int count, double dt, double time)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world->doesRender)
            for (int i = 0; i < world->draw3dCount; i++)
                world->draw3dSystems[i](world, dt, time);
    }
}

void Worlds_Draw2d(World **worlds, int count, double dt, double time)
{
    for (int w = count - 1; w >= 0; w--)
    {
        World *world = worlds[w];
        if (world->doesRender)
            for (int i = 0; i < world->draw2dCount; i++)
                world->draw2dSystems[i](world, dt, time);
    }
}

int Sol_Create_EntNoXform(World *world)
{
    int id = 0;
    while (id <= world->maxEntities && world->masks[id] != 0)
        id++;

    SolActive *sol_active      = Sol_Comp_Add(world, id, SolActive);
    sol_active->active_at_tick = world->currentTick;
    sol_active->time_activated = solState.gameTime;

    return id;
}

int Sol_Create_Ent(World *world)
{
    int id = 0;
    while (id <= world->maxEntities && world->masks[id] != 0)
        id++;

    SolActive *sol_active      = Sol_Comp_Add(world, id, SolActive);
    sol_active->active_at_tick = world->currentTick;
    sol_active->time_activated = solState.gameTime;

    SolXform *xform = Sol_Comp_Add(world, id, SolXform);
    xform->rot      = (versors){0.0f, 0.0f, 0.0f, 1.0f};
    xform->last_rot = (versors){0.0f, 0.0f, 0.0f, 1.0f};
    xform->draw_rot = (versors){0.0f, 0.0f, 0.0f, 1.0f};
    xform->sca      = (vec3s){1.0f, 1.0f, 1.0f};
    xform->last_sca = (vec3s){1.0f, 1.0f, 1.0f};
    xform->draw_sca = (vec3s){1.0f, 1.0f, 1.0f};

    return id;
}
