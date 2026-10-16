/*
 * File: world.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * World!
 */
#include "world.h"
#include "components.h"
#include "systems.h"
#include "sol_core.h"
#include "platform/platform.h"
#include <omp.h>

#define AS_FN(ENUM, PHASE, FN) void FN(World *world, double dt);
SOL_SYSTEMS(AS_FN)
#undef AS_FN

typedef struct
{
    Phase phase;
    UpdateFn update;
} SysUpdate;
const SysUpdate sys_updates[] = {
#define AS_UPDATES(ENUM, PHASE, FN) [ENUM] = {PHASE, FN},
    SOL_SYSTEMS(AS_UPDATES)
#undef AS_UPDATES
};

World *World_Create()
{
    World *world = calloc(1, sizeof(World));
    if (!world)
        return NULL;
    int index              = solState.worldCount++;
    world->timescale       = 1.0f;
    world->maxEntities     = MAX_ENTS;
    world->doesSimulate    = true;
    world->doesRender      = true;
    world->index           = index;
    solState.worlds[index] = world;

    // for (int i = 0; i < ONSKIND_COUNT; i++)
    //     solb_init(world->ons[i], 1);

    // solb_push(world->ons[ONSKIND_DESTROY], Sol_Inv_AllDirty);

    Sol_Comp_InitAll(world);
    World_InitSingletons(world);

    return world;
}

World *World_Create_AllSys()
{
    World *world = World_Create();
    if (!world)
        return NULL;

    for (int sys = 0; sys < SYS_COUNT; sys++)
        Sol_Sys_Add(world, (SolSys)sys);

    return world;
}

void World_Destroy(World *world)
{
    if (world)
    {
        World_DeinitSingletons(world); // frees internal solb_ buffers, structs still intact
        Sol_Comp_FreeAll(world);

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

void Sol_Sys_Add(World *world, SolSys system)
{
    if (world->system_mask & BITC(system))
        return;
    const SysUpdate sys = sys_updates[system];
    if (sys.update)
    {
        switch (sys.phase)
        {
        case PHASE_PRE:
            WAddTick(world) = sys.update;
            break;
        case PHASE_STEP:
            WAddStep(world) = sys.update;
            break;
        case PHASE_POST:
            WAddPosttick(world) = sys.update;
            break;
        case PHASE_RENDER3:
            WAdd3d(world) = sys.update;
            break;
        case PHASE_RENDER2:
            WAdd2d(world) = sys.update;
            break;
        }
    }
    world->system_mask |= BITC(system);
}

void Sol_Sys_Add_Ui(World *world)
{    
    Sol_Sys_Add(world, SYS_INTERACT);
    Sol_Sys_Add(world, SYS_DRAG);
    Sol_Sys_Add(world, SYS_HOOK);
    Sol_Sys_Add(world, SYS_BODY2);
    Sol_Sys_Add(world, SYS_UI);
    Sol_Sys_Add(world, SYS_PARENT);
    Sol_Sys_Add(world, SYS_VIEW2_DRAW);
}

static void RemoveSystemFromList(UpdateFn *list, int *count, UpdateFn sys)
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

void Sol_Sys_Remove(World *world, SolSys system)
{
    // if (!(world->system_mask & BITC(system)))
    //     return;

    // for (int i = 0; i < SYSTEMUPDATEDEF_COUNT; i++)
    // {
    //     UpdateFn update_fn = system_inits[system].update[i].update;
    //     if (update_fn)
    //     {
    //         switch (system_inits[system].update[i].phase)
    //         {
    //         case UPDATEPHASE_TICK:
    //             RemoveSystemFromList(world->tickSystems, &world->tickCount, update_fn);
    //             break;
    //         case UPDATEPHASE_STEP:
    //             RemoveSystemFromList(world->stepSystems, &world->stepCount, update_fn);
    //             break;
    //         case UPDATEPHASE_POSTTICK:
    //             RemoveSystemFromList(world->posttickSystems, &world->posttickCount, update_fn);
    //             break;
    //         case UPDATEPHASE_RENDER3:
    //             RemoveSystemFromList(world->draw3dSystems, &world->draw3dCount, update_fn);
    //             break;
    //         case UPDATEPHASE_RENDER2:
    //             RemoveSystemFromList(world->draw2dSystems, &world->draw2dCount, update_fn);
    //             break;
    //         }
    //     }
    // }

    // world->system_mask &= ~BITC(system);
}

void Worlds_Tick(World **worlds, int count, double dt)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world && world->doesSimulate)
        {
            double world_dt = dt * world->timescale;
            world->dt       = world_dt;
            world->fdt      = (float)world_dt;
            world->currentTick++;
            world->tickTime += world_dt;

            for (int i = 0; i < world->tickCount; i++)
                world->tickSystems[i](world, world_dt);
        }
    }
}

void Worlds_Step(World **worlds, int count, double dt)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world && world->doesSimulate)
        {
            double world_dt = dt * world->timescale;
            world->currentStep++;
            world->stepTime += world_dt;

            for (int i = 0; i < world->stepCount; i++)
                world->stepSystems[i](world, world_dt);
        }
    }
}

void Worlds_PostTick(World **worlds, int count, double dt)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world && world->doesSimulate)
        {
            double world_dt = dt * world->timescale;
            for (int i = 0; i < world->posttickCount; i++)
                world->posttickSystems[i](world, world_dt);
        }
    }
}

void Worlds_Draw3d(World **worlds, int count, double dt)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world && world->doesRender)
        {
            double world_dt = dt * world->timescale;
            for (int i = 0; i < world->draw3dCount; i++)
                world->draw3dSystems[i](world, world_dt);
        }
    }
}

void Worlds_Draw2d(World **worlds, int count, double dt)
{
    for (int w = count - 1; w >= 0; w--)
    {
        World *world = worlds[w];
        if (world && world->doesRender)
        {
            double world_dt = dt * world->timescale;
            for (int i = 0; i < world->draw2dCount; i++)
                world->draw2dSystems[i](world, world_dt);
        }
    }
}

int Sol_Create_Ent(World *world, vec3s pos)
{
    int id = 1;
    while (id < world->maxEntities && world->masks[id] != 0)
        id++;

    if (id >= world->maxEntities)
        return 0;

    int dense                  = world->entCount++;
    world->sparse[id]          = dense;
    world->dense[dense]        = id;
    ScActive *sol_active       = Sol_Comp_Add(world, id, ScActive);
    sol_active->active_at_tick = world->currentTick;
    sol_active->time_activated = world->tickTime;
    Xform_SetAll(world, id, pos, (versors){0.0f, 0.0f, 0.0f, 1.0f}, (vec3s){1.0f, 1.0f, 1.0f});
    world->xform.home_pos[id] = pos;

    Sol_Debug_Add("Entities", world->entCount);

    return id;
}

void Sol_Destroy_Ent(World *w, int entId)
{
    if (!w || entId < 1 || entId > w->maxEntities)
        return;

    Sol_Event_Push(w, EVENTKIND_DESTROY, (SolEvent){.target = entId, .pos = w->xform.pos[entId]});
    // for (int i = 0; i < solb_count(w->ons[ONSKIND_DESTROY]); i++)
    // {
    //     if (w->ons[ONSKIND_DESTROY][i])
    //         w->ons[ONSKIND_DESTROY][i](w);
    // }
    int children[64];
    int child_count;
    do
    {
        child_count = Sol_Parent_GetChildren(w, entId, children, (int)(sizeof(children) / sizeof(children[0])));
        for (int i = 0; i < child_count; i++)
            Sol_Destroy_Ent(w, children[i]);
    } while (child_count != 0);

    u64 mask = w->masks[entId];
    while (mask != 0)
    {
#if defined(_MSC_VER) && !defined(__clang__)
        unsigned long compEnum;
        _BitScanForward64(&compEnum, mask);
#else
        int compEnum = __builtin_ctzll(mask);
#endif
        Sol_Comp_RemE(w, entId, compEnum);
        mask &= mask - 1; // Clear lowest bit
    }
    int removedDense       = w->sparse[entId];
    int lastEntity         = w->dense[w->entCount - 1];
    w->dense[removedDense] = lastEntity;
    w->sparse[lastEntity]  = removedDense;
    w->entCount--;
}

int Sol_Duplicate_Ent(World *world, int id, World *target_world, vec3s pos)
{
    int new_id = Sol_Create_Ent(target_world, pos);
    for (int i = 0; i < COMPONENT_COUNT; i++)
    {
        if (Sol_Comp_HasE(world, id, i))
        {
            BaseSparseSet *src_set = (BaseSparseSet *)world->components[i];
            size_t data_size       = src_set->data_size;
            int src_dense          = src_set->sparse[id];
            void *old_comp         = ((char *)src_set->data) + (src_dense * data_size);

            void *new_comp = Sol_Comp_AddE(target_world, new_id, i);
            if (old_comp && new_comp)
                memcpy(new_comp, old_comp, data_size);
        }
    }

    return new_id;
}
