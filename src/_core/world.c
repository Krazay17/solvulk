/*
 * File: world.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * World!
 */
#include "sol_core.h"

typedef struct
{
    SystemInit init;
    SystemFunc tick;
    SystemFunc step;
    SystemFunc draw2d;
    SystemFunc draw3d;
} SystemConfig;

static SystemInit init_system[WORLD_SYS_COUNT] = {
    [WORLD_SYS_EVENT]      = Sol_Event_Init,
    [WORLD_SYS_TIMER]      = Sol_Timer_Init,
    [WORLD_SYS_PHYSX]      = Sol_Physx_Init,
    [WORLD_SYS_PARENT]     = Sol_Parent_Init,
    [WORLD_SYS_CONTROLLER] = Sol_Controller_Init,
    [WORLD_SYS_XFORM]      = Sol_Xform_Init,
    [WORLD_SYS_INTERACT]   = Sol_Interact_Init,
    [WORLD_SYS_MOVEMENT]   = Sol_Movement_Init,
    [WORLD_SYS_COMBAT]     = Sol_Combat_Init,
    [WORLD_SYS_ABILITY]    = Sol_Ability_Init,
    [WORLD_SYS_BUFF]       = Sol_Buff_Init,
    [WORLD_SYS_VITAL]      = Sol_Vital_Init,
    [WORLD_SYS_MODEL]      = Sol_Model_Init,
    [WORLD_SYS_UI]         = Sol_Ui_Init,
    [WORLD_SYS_LINE]       = Sol_Line_Init,
    [WORLD_SYS_EMITTER]    = Sol_Emitter_Init,
    [WORLD_SYS_SHAPE]      = Sol_Shape_Init,
    [WORLD_SYS_PICKUP]     = Sol_Pickup_Init,
    [WORLD_SYS_CAM]        = Sol_Cam_Init,
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
    Sol_Event_Clear(world);
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

void World_Draw3d(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->draw3dCount; i++)
    {
        world->draw3dSystems[i](world, dt, time);
    }
    Flush_Queue();
}

void World_Draw2d(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->draw2dCount; i++)
    {
        world->draw2dSystems[i](world, dt, time);
    }
}

void World_System_Add(World *world, WorldSystem system)
{
    if (world && init_system[system])
        init_system[system](world);
}

int Sol_Create_Ent(World *world)
{
    if (!world)
        return 0;
    for (int i = 1; i < MAX_ENTS; i++)
    {
        if (!world->actives[i])
        {
            world->actives[i]                           = true;
            world->masks[i]                             = HAS_NONE;
            world->activeEntities[world->activeCount++] = i;
            Sol_Debug_Add("Entities", world->activeCount);
            return i;
        }
    }
    return -1;
}

void Sol_Destroy_Ent(World *world, int id)
{
    /*
        id = 2
        count = 4
        activeEnts [0][1][2][3]
                   '2''1''3''6'
        index 0 = index[count - 1]
        // Puts last ent into the slot this ent was in, -1 because count is +1 offset from array index
        count-- // lowers count to filter ent moved to last
        activeEnts [0][1][2]
                   '6''1''3'
    */
    world->actives[id] = false;
    world->masks[id]   = 0;
    for (int i = 0; i < world->activeCount; i++)
    {
        if (world->activeEntities[i] == id)
        {
            world->activeEntities[i] = world->activeEntities[--world->activeCount];
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

int Sol_World_GetEntCount(World *world)
{
    return world->activeCount;
}
