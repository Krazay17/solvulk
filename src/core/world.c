/*
 * File: world.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * World!
 */
#include "sol/sol.h"
#include "sol_core.h"

static u32 world_count = 0;

static SystemFunc init_system[WORLD_SYS_COUNT] = {
#define AS_ARRAY_MAPPING(enum_name, init_func) [enum_name] = init_func,
    SOL_SYSTEM_LIST(AS_ARRAY_MAPPING)
#undef AS_ARRAY_MAPPING
};

// static SystemFunc init_system[WORLD_SYS_COUNT] = {
//     [WORLD_SYS_EVENT]        = Sol_Event_Init,
//     [WORLD_SYS_TIMER]        = Sol_Timer_Init,
//     [WORLD_SYS_PHYSX]        = Sol_Physx_Init,
//     [WORLD_SYS_PARENT]       = Sol_Parent_Init,
//     [WORLD_SYS_CONTROLLER]   = Sol_Controller_Init,
//     [WORLD_SYS_XFORM]        = Sol_Xform_Init,
//     [WORLD_SYS_INTERACT]     = Sol_Interact_Init,
//     [WORLD_SYS_MOVEMENT]     = Sol_Movement_Init,
//     [WORLD_SYS_COMBAT]       = Sol_Combat_Init,
//     [WORLD_SYS_ABILITY]      = Sol_Ability_Init,
//     [WORLD_SYS_BUFF]         = Sol_Buff_Init,
//     [WORLD_SYS_VITAL]        = Sol_Vital_Init,
//     [WORLD_SYS_MODEL]        = Sol_Model_Init,
//     [WORLD_SYS_LINE]         = Sol_Line_Init,
//     [WORLD_SYS_EMITTER]      = Sol_Emitter_Init,
//     [WORLD_SYS_SHAPE]        = Sol_Shape_Init,
//     [WORLD_SYS_PICKUP]       = Sol_Pickup_Init,
//     [WORLD_SYS_VIEW]         = Sol_View_Init,
//     [WORLD_SYS_OWNER]        = Sol_Owner_Init,
//     [WORLD_SYS_AICONTROLLER] = Sol_Ai_Init,
//     [WORLD_SYS_REPLICATION]  = Sol_Replication_Init,
//     [WORLD_SYS_BODY2]        = Sol_Body2d_Init,
//     [WORLD_SYS_VIEW2D]       = Sol_View2d_Init,
//     [WORLD_SYS_PROJECTILE]   = Sol_Projectile_Init,
//     [WORLD_SYS_ITEM]         = Sol_Item_Init,
//     [WORLD_SYS_RIBBON]       = Sol_Ribbon_Init,
//     [WORLD_SYS_CHAINHIT]     = Sol_Chainhit_Init,
//     [WORLD_SYS_AUDIO]        = Sol_World_Audio_Init,
// };

World *World_Create(WorldKind kind)
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->doesSimulate                      = true;
        world->doesRender                        = true;
        world->doesReplicate                     = false;
        world->systemBits                        = 0;
        world->playerID                          = -1;
        world->kind                              = kind;
        world->worldId                           = world_count++;
        solEngine.worlds[solEngine.worldCount++] = world;
    }

    return world;
}

World *World_Create_Default(WorldKind kind)
{
    World *world = World_Create(kind);
    if (world)
    {
        switch (kind)
        {
        case WORLDKIND_MENU:
            World_System_Add(world, WORLD_SYS_EVENT);
            World_System_Add(world, WORLD_SYS_XFORM);
            World_System_Add(world, WORLD_SYS_BODY2);
            World_System_Add(world, WORLD_SYS_PARENT);
            World_System_Add(world, WORLD_SYS_INTERACT);
            World_System_Add(world, WORLD_SYS_ITEM);
            World_System_Add(world, WORLD_SYS_VIEW2D);
            World_System_Add(world, WORLD_SYS_MODEL);
            World_System_Add(world, WORLD_SYS_VIEW);
            break;

        default:
            for (int i = 0; i < WORLD_SYS_COUNT; i++)
                World_System_Add(world, i);
        }
    }
    return world;
}

void World_Destroy(World *world)
{
    if (world)
    {
        for (int i = 0; i < world->deinitCount; i++)
        {
            if (world->deinitSystems[i])
                world->deinitSystems[i](world);
        }
        free(world);
        world_count--;
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
            for (int w = 0; w < world->tickCount; w++)
                world->tickSystems[w](world, dt, time);
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
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world->doesRender)
            for (int i = 0; i < world->draw2dCount; i++)
                world->draw2dSystems[i](world, dt, time);
    }
}

void World_System_Add(World *world, WorldSystem system)
{
    if (world && init_system[system])
        init_system[system](world);
    world->systemBits |= BITC(system);
}

int Sol_Create_Ent(World *world, u32 id)
{
    if (!world)
        return 0;
    if (!id)
        for (int i = 2; i < MAX_ENTS; i++)
        {
            if (!(world->masks[i] & BITC(HAS_ACTIVE)))
            {
                id = i;
                break;
            }
        }
    u32 idx = Sol_GetEntIndex(id);
    u32 gen = Sol_GetEntGen(id);
    gen++;
    world->gens[id] = Sol_CreateEntGen(idx, gen);

    world->masks[id]                            = BITC(HAS_ACTIVE);
    world->activeEntities[world->activeCount++] = id;
    world->xforms[id]                           = (CompXform){
        .pos       = {0.0f, 0.0f, 0.0f},
        .lastPos   = {0.0f, 0.0f, 0.0f},
        .drawPos   = {0.0f, 0.0f, 0.0f},
        .quat      = (versors){{0.0f, 0.0f, 0.0f, 1.0f}},
        .lastQuat  = (versors){{0.0f, 0.0f, 0.0f, 1.0f}},
        .drawQuat  = (versors){{0.0f, 0.0f, 0.0f, 1.0f}},
        .scale     = {1.0f, 1.0f, 1.0f},
        .lastScale = {1.0f, 1.0f, 1.0f},
        .drawScale = {1.0f, 1.0f, 1.0f},
    };
    Sol_Debug_Add("Entities", (float)world->activeCount);
    return id;
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
    world->masks[id]       = 0;
    world->flags[id].flags = 0;
    world->ekinds[id]      = 0;
    for (int i = 0; i < world->activeCount; i++)
    {
        if (world->activeEntities[i] == id)
        {
            world->activeEntities[i] = world->activeEntities[--world->activeCount];
            Sol_Debug_Add("Entities", (float)world->activeCount);
            break;
        }
    }
}

void Sol_Flags_Add(World *world, int id, EFlag flags)
{
    world->flags[id].flags |= flags;
}

void Sol_Flags_Remove(World *world, int id, EFlag flags)
{
    world->flags[id].flags &= ~flags;
}

int Sol_World_GetEntCount(World *world)
{
    return world->activeCount;
}
void Sol_World_SetReplicates(World *world, bool active)
{
    world->doesReplicate = active;
}
void Sol_World_SetActive(World *world)
{
    solEngine.activeWorld   = world;
    solEngine.activeWorldId = world->worldId;
}
void Sol_World_SetTracker(World *world, int id, World *otherWorld, int otherId)
{
    world->trackers[id].world = otherWorld;
    world->trackers[id].entId = otherId;
    world->masks[id] |= BITC(HAS_TRACKER);
}