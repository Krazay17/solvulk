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
#include "spatial_grid.h"
#include "platform/platform.h"
#include <omp.h>

// Systems
void Sol_Test(World *world, double dt);
void Cmd_Update(World *world, double dt);
void Player_Update(World *world, double dt);
void Interact_Update(World *world, double dt);
void Parent_Update(World *world, double dt);
void Abilitybar_Update(World *world, double dt);

void Move3_Update(World *world, double dt);
void Move2_Update(World *world, double dt);

void Body3_Update(World *world, double dt);
void Body2_Step(World *world, double dt);

void Buff_Update(World *world, double dt);
void Ability_Update(World *world, double dt);
void Projectile_Step(World *world, double dt);
void Zone_Update(World *world, double dt);
void Combat_Update(World *world, double dt);
void Ai_Step(World *world, double dt);
void Interact_Step(World *world, double dt);

void Ref_Update(World *world, double dt);
void Fx_Update(World *world, double dt);
void Hook_Tick(World *world, double dt);
void Anim_Tick(World *world, double dt);
void Facing_Tick(World *world, double dt);
void Camera_Tick(World *world, double dt);
void Emitter_Update(World *world, double dt);
void Timer_Update(World *world, double dt);

void Weapon_Update(World *world, double dt);
void Particle_Draw(World *world, double dt);
void Buff_Draw(World *world, double dt);
void Scoreboard_Draw(World *world, double dt);
void Model_Render(World *world, double dt);
void Ability_Draw(World *world, double dt);
void View3_Draw(World *world, double dt);
void View2_Draw(World *world, double dt);
void Debug_Tick(World *world, double dt);
void Debug_Draw3(World *world, double dt);
void Debug_Draw2(World *world, double dt);
void Ribbon_Update(World *world, double dt);
void Ribbon_Update(World *world, double dt);

#define SYSTEMUPDATEDEF_COUNT 3
const struct SystemDef
{
    struct
    {
        SystemUpdate update;
        UpdatePhase phase;
    } update[SYSTEMUPDATEDEF_COUNT];
} system_inits[WORLDSYS_COUNT] = {
    [WORLDSYS_TIMER] = {.update = {Timer_Update, UPDATEPHASE_TICK}},

    [WORLDSYS_CMD]        = {.update = {Cmd_Update, UPDATEPHASE_TICK}},
    [WORLDSYS_PLAYER]     = {.update = {Player_Update, UPDATEPHASE_TICK}},
    [WORLDSYS_INTERACT]   = {.update = {{Interact_Update, UPDATEPHASE_TICK}, {Interact_Step, UPDATEPHASE_STEP}}},
    [WORLDSYS_ABILITYBAR] = {.update = Abilitybar_Update, UPDATEPHASE_TICK},

    [WORLDSYS_TEST] = {.update = {{Sol_Test, UPDATEPHASE_TICK}}},

    [WORLDSYS_BUFF]       = {.update = {{Buff_Update, UPDATEPHASE_STEP}, {Buff_Draw, UPDATEPHASE_RENDER3}}},
    [WORLDSYS_MOVE3]      = {.update = {Move3_Update, UPDATEPHASE_STEP}},
    [WORLDSYS_MOVE2]      = {.update = {Move2_Update, UPDATEPHASE_STEP}},
    [WORLDSYS_BODY3]      = {.update = {Body3_Update, UPDATEPHASE_STEP}},
    [WORLDSYS_BODY2]      = {.update = {Body2_Step, UPDATEPHASE_STEP}},
    [WORLDSYS_ABILITY]    = {.update = {{Ability_Update, UPDATEPHASE_STEP}, {Ability_Draw, UPDATEPHASE_RENDER3}}},
    [WORLDSYS_PROJECTILE] = {.update = {Projectile_Step, UPDATEPHASE_STEP}},
    [WORLDSYS_ZONE]       = {.update = {Zone_Update, UPDATEPHASE_STEP}},
    [WORLDSYS_COMBAT]     = {.update = {Combat_Update, UPDATEPHASE_STEP}},
    [WORLDSYS_AI]         = {.update = {Ai_Step, UPDATEPHASE_STEP}},

    [WORLDSYS_WEAPON] = {.update = {Weapon_Update, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_REF]    = {.update = {Ref_Update, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_FX]     = {.update = {Fx_Update, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_HOOK]   = {.update = {Hook_Tick, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_FACING] = {.update = {Facing_Tick, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_CAMERA] = {.update = {Camera_Tick, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_ANIM]   = {.update = {Anim_Tick, UPDATEPHASE_POSTTICK}},
    [WORLDSYS_PARENT] = {.update = {Parent_Update, UPDATEPHASE_POSTTICK}},

    [WORLDSYS_EMITTER] = {.update = {{Emitter_Update, UPDATEPHASE_POSTTICK}, {Particle_Draw, UPDATEPHASE_RENDER3}}},
    [WORLDSYS_RIBBON]  = {.update = {{Ribbon_Update, UPDATEPHASE_RENDER3}}},

    [WORLDSYS_MODEL] = {.update = {Model_Render, UPDATEPHASE_RENDER3}},
    [WORLDSYS_VIEW3] = {.update = {View3_Draw, UPDATEPHASE_RENDER3}},

    [WORLDSYS_VIEW2]      = {.update = {View2_Draw, UPDATEPHASE_RENDER2}},
    [WORLDSYS_SCOREBOARD] = {.update = {Scoreboard_Draw, UPDATEPHASE_RENDER2}},

    [WORLDSYS_DEBUG] =
        {
            .update[0] = {Debug_Tick, UPDATEPHASE_TICK},
            .update[1] = {Debug_Draw3, UPDATEPHASE_RENDER3},
            .update[2] = {Debug_Draw2, UPDATEPHASE_RENDER2},
        },
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

    Sol_Comp_InitAll(world);
    World_InitSingletons(world);

    return world;
}

World *World_Create_AllSys()
{
    World *world = World_Create();
    if (!world)
        return NULL;

    for (int sys = 0; sys < WORLDSYS_COUNT; sys++)
        Sol_Sys_Add(world, (WorldSystems)sys);

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

void Sol_Sys_Add(World *world, WorldSystems system)
{
    if (world->system_mask & BITC(system))
        return;
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

void Worlds_Event_Clear(World **worlds, int count)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (world && world->doesSimulate)
        {
            SlEvent *single = Sol_Comp_Get(world, 0, SlEvent);
            solb_set_count(single->events, 0);
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

void SlEvent_Init(World *world, SlEvent *self)
{
    solb_init(self->events, 64);
}
void SlEvent_Deinit(SlEvent *self)
{
    solb_free(self->events);
}

void SlEmitter_Init(World *world, SlEmitter *self)
{
    solb_init(self->emitters, 128);
    solb_init(self->particles, 512);
}

void SlEmitter_Deinit(SlEmitter *self)
{
    solb_free(self->emitters);
    solb_free(self->particles);
}

void SlSpatial_Init(World *world, SlSpatial *self)
{
    solb_init(self->contacts, 4096);
    solb_init(self->tris_static, 4096);
    solb_init(self->build_ids, 64);
    solb_init(self->build_mins, 64);
    solb_init(self->build_maxs, 64);

    int max_threads = omp_get_max_threads();
    solb_init(self->threadContacts, max_threads);
    solb_set_count(self->threadContacts, max_threads);
    solb_init(self->threadIds, max_threads);
    solb_set_count(self->threadIds, max_threads);

    for (int i = 0; i < max_threads; i++)
    {
        solb_init(self->threadContacts[i].contacts, 256);
        solb_init(self->threadIds[i].ids, 32);
    }

    {
        float cell_size    = 5.0f;
        self->grid_dynamic = malloc(sizeof(SpatialGrid));
        vec3s min          = {-512.0f, -512.0f, -32.0f};
        vec3s max          = {512.0f, 512.0f, 256.0f};
        SpatialGrid_Init(self->grid_dynamic, min, max, cell_size);
    }

    {
        float cell_size   = 5.0f;
        self->grid_static = malloc(sizeof(SpatialGrid));
        vec3s min         = {-512.0f, -512.0f, -32.0f};
        vec3s max         = {512.0f, 512.0f, 256.0f};
        SpatialGrid_Init(self->grid_static, min, max, cell_size);
    }
}

void SlSpatial_Deinit(SlSpatial *self)
{
    SpatialGrid_Deinit(self->grid_dynamic);
    SpatialGrid_Deinit(self->grid_static);
    solb_free(self->contacts);
    solb_free(self->threadIds);
    solb_free(self->tris_static);
    solb_free(self->build_ids);
    solb_free(self->build_mins);
    solb_free(self->build_maxs);
}

void SlContacts2_Init(World *world, SlContacts2 *self)
{
    solb_init(self->contacts, 16);
}

void SlContacts2_Deinit(SlContacts2 *self)
{
    solb_free(self->contacts);
}

void SlDebug_Init(World *world, SlDebug *self)
{
    solb_init(self->lines, 32);
    solb_init(self->spheres, 32);
}

void SlDebug_Deinit(SlDebug *self)
{
}

void Sol_Event_Push(World *world, EventKind kind, SolEvent event)
{
    SlEvent *single = Sol_Comp_Get(world, 0, SlEvent);
    event.kind      = kind;
    solb_push(single->events, event);
}