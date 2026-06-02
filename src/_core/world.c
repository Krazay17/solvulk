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

static u32 worldId;

static SystemInit init_system[WORLD_SYS_COUNT] = {
    [WORLD_SYS_EVENT]        = Sol_Event_Init,
    [WORLD_SYS_TIMER]        = Sol_Timer_Init,
    [WORLD_SYS_PHYSX]        = Sol_Physx_Init,
    [WORLD_SYS_PARENT]       = Sol_Parent_Init,
    [WORLD_SYS_CONTROLLER]   = Sol_Controller_Init,
    [WORLD_SYS_XFORM]        = Sol_Xform_Init,
    [WORLD_SYS_INTERACT]     = Sol_Interact_Init,
    [WORLD_SYS_MOVEMENT]     = Sol_Movement_Init,
    [WORLD_SYS_CONTACT]      = Sol_Contact_Init,
    [WORLD_SYS_COMBAT]       = Sol_Combat_Init,
    [WORLD_SYS_ABILITY]      = Sol_Ability_Init,
    [WORLD_SYS_BUFF]         = Sol_Buff_Init,
    [WORLD_SYS_VITAL]        = Sol_Vital_Init,
    [WORLD_SYS_MODEL]        = Sol_Model_Init,
    [WORLD_SYS_UI]           = Sol_Ui_Init,
    [WORLD_SYS_LINE]         = Sol_Line_Init,
    [WORLD_SYS_EMITTER]      = Sol_Emitter_Init,
    [WORLD_SYS_SHAPE]        = Sol_Shape_Init,
    [WORLD_SYS_PICKUP]       = Sol_Pickup_Init,
    [WORLD_SYS_VIEW]         = Sol_View_Init,
    [WORLD_SYS_OWNER]        = Sol_Owner_Init,
    [WORLD_SYS_AICONTROLLER] = Sol_AiController_Init,
    [WORLD_SYS_REPLICATION]  = Sol_Replication_Init,
    [WORLD_SYS_BODY2]        = Sol_Body2d_Init,
    [WORLD_SYS_VIEW2D]       = Sol_View2d_Init,
};

World *World_Create(WorldKind kind)
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->doesSimulate                = true;
        world->doesRender                  = true;
        world->playerID                    = -1;
        world->kind                        = kind;
        world->worldId                     = worldId++;
        SolState *state                    = Sol_GetState();
        state->worlds[state->worldCount++] = world;
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
            // case WORLDKIND_MENU:
            //     World_System_Add(world, WORLD_SYS_XFORM);
            //     World_System_Add(world, WORLD_SYS_EVENT);
            //     World_System_Add(world, WORLD_SYS_INTERACT);
            //     World_System_Add(world, WORLD_SYS_SHAPE);
            //     World_System_Add(world, WORLD_SYS_UI);
            //     World_System_Add(world, WORLD_SYS_VIEW);
            //     break;

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
        free(world);
}

void World_Step(World *world, double dt, double time)
{
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

void World_Tick(World *world, double dt, double time)
{
    for (int i = 0; i < world->tickCount; i++)
    {
        world->tickSystems[i](world, dt, time);
    }
}

void World_Draw3d(World *world, double dt, double time)
{
    for (int i = 0; i < world->draw3dCount; i++)
    {
        world->draw3dSystems[i](world, dt, time);
    }
}

void World_Draw2d(World *world, double dt, double time)
{
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

int Sol_Create_Ent(World *world, u32 id)
{
    if (!world)
        return 0;
    if (!id)
        for (int i = 2; i < MAX_ENTS; i++)
        {
            if (!(world->masks[i] & HAS_ACTIVE))
            {
                id = i;
                break;
            }
        }
    world->masks[id]                            = HAS_ACTIVE;
    world->activeEntities[world->activeCount++] = id;
    world->xforms[id]                           = (CompXform){
        .pos       = {0, 0, 0},
        .lastPos   = {0, 0, 0},
        .drawPos   = {0, 0, 0},
        .quat      = (versors){{0, 0, 0, 1}},
        .lastQuat  = (versors){{0, 0, 0, 1}},
        .drawQuat  = (versors){{0, 0, 0, 1}},
        .scale     = {1, 1, 1},
        .lastScale = {1, 1, 1},
        .drawScale = {1, 1, 1},
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
void World_SetDoesrender(World *world, bool doesRender)
{
    world->doesRender = doesRender;
}
void Sol_World_SetReplicates(World *world, bool active)
{
    world->doesReplicate = active;
}
void Sol_World_SetActive(World *world)
{
    Sol_GetState()->activeWorld   = world;
    Sol_GetState()->activeWorldId = world->worldId;
}