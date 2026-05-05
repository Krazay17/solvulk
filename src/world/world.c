#include "sol_core.h"

static SystemConfig world_systems[WORLD_SYS_COUNT] = {
    [WORLD_SYS_TIMER]      = {.init = Sol_Timer_Init, .tick = Sol_Timer_Tick},
    [WORLD_SYS_PHYSX]      = {.init = Sol_Physx_Init, .step = Sol_Physx_Step},
    [WORLD_SYS_CONTROLLER] = {.init = Sol_Controller_Init, .tick = Sol_Controller_Tick},
    [WORLD_SYS_XFORM]      = {.init = Sol_Xform_Init},
    [WORLD_SYS_INTERACT]   = {.init = Sol_Interact_Init, .tick = System_Interact_Tick},
    [WORLD_SYS_MOVEMENT]   = {.init = Sol_Movement_Init, .step = Sol_System_Movement_3d_Step},
    [WORLD_SYS_COMBAT]     = {.init = Sol_Ability_Init, .tick = Sol_Ability_Tick, .step = Sol_Ability_Step},
    [WORLD_SYS_BUFF]       = {.init = Sol_Buff_Init, .step = Sol_Buff_Step},
    [WORLD_SYS_VITAL]      = {.init = Sol_Vital_Init, .step = Sol_Vital_Step, .draw3d = Sol_Vital_Draw},
    [WORLD_SYS_MODEL]      = {.init = Sol_Model_Init, .draw3d = Sol_Model_Draw},
    [WORLD_SYS_UI]         = {.init = Sol_Ui_Init, .draw2d = Sol_Ui_Draw},
    [WORLD_SYS_LINE]       = {.init = Sol_Line_Init, .tick = Sol_Line_Tick, .draw3d = Sol_Line_Draw},
    [WORLD_SYS_EMITTER]    = {.init = Emitter_Init, .tick = Emitter_Tick, .draw3d = Emitter_Draw, .step = Emitter_Step},
    [WORLD_SYS_SPHERE]     = {.init = Sol_Sphere_Init, .draw3d = Sol_Sphere_Draw, .step = Sol_Sphere_Step},
    [WORLD_SYS_PICKUP]     = {.step = Sol_Pickup_Step},
    [WORLD_SYS_CAM]        = {.draw2d = Sol_Crosshair_Draw},
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

    Event_Init(world);

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
    if (!world)
        return;
    if (world_systems[system].init)
        world_systems[system].init(world);
    if (world_systems[system].step)
        world->stepSystems[world->stepCount++] = world_systems[system].step;
    if (world_systems[system].tick)
        world->tickSystems[world->tickCount++] = world_systems[system].tick;
    if (world_systems[system].draw3d)
        world->draw3dSystems[world->draw3dCount++] = world_systems[system].draw3d;
    if (world_systems[system].draw2d)
        world->draw2dSystems[world->draw2dCount++] = world_systems[system].draw2d;
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
