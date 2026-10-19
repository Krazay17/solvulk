/*
 * File: component.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-04
 *
 */
#include "components.h"
#include "world.h"
#include "spatial_grid.h"

const ScParent parent_base = {
    .localScale = {1, 1, 1},
    .localQuat  = {0, 0, 0, 1},
};

void Add_None(World *world, int id, void *c)
{
}

void ScInventory_Init(World *world, int id, ScInventory *c)
{
    c->dirty       = true;
    c->dirty_equip = true;
    memset(c->equipped_items, -1, sizeof(c->equipped_items));
}

void ScAnim_Init(World *world, int id, ScAnim *c)
{
    c->layers[0] = (AnimLayer){.animId = 0, .currentAnim = 0, .blendFactor = 1.0f, .weight = 1.0f};
    c->layers[1] = (AnimLayer){.animId = -1, .currentAnim = 0, .blendFactor = 1.0f, .weight = 1.0f};
    c->layers[2] = (AnimLayer){.animId = -1, .currentAnim = 0, .blendFactor = 1.0f, .weight = 1.0f};
    c->layers[3] = (AnimLayer){.animId = -1, .currentAnim = 0, .blendFactor = 1.0f, .weight = 1.0f};
}

void ScParent_Init(World *world, int id, ScParent *c)
{
    c->localScale.x = 1.0f;
    c->localScale.y = 1.0f;
    c->localScale.z = 1.0f;
}

void SlRules_Init(World *world, SlRules *self)
{
}
void SlRules_Deinit(SlRules *self)
{
}

void SlEvent_Init(World *world, SlEvent *self)
{
    solb_init(self->events, 64);
    for (int i = 0; i < EVENTKIND_COUNT; i++)
        solb_init(self->handlers[i], 4);
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
        float cell_size    = 10.0f;
        self->grid_dynamic = malloc(sizeof(SpatialGrid));
        vec3s min          = {-512.0f, -512.0f, -32.0f};
        vec3s max          = {512.0f, 512.0f, 256.0f};
        SpatialGrid_Init(self->grid_dynamic, min, max, cell_size);
    }

    {
        float cell_size   = 10.0f;
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

static BaseSparseSet *SparseSet_Alloc(size_t data_size, int max_ents)
{
    BaseSparseSet *set = calloc(1, sizeof(*set));
    if (!set)
        return NULL;
    set->data_size = data_size;
    set->sparse    = malloc((size_t)max_ents * sizeof(*set->sparse));
    if (!set->sparse)
    {
        free(set);
        return NULL;
    }
    return set;
}

void Sol_Comp_InitAll(World *world)
{
#define INIT_COMPONENT(type, idx, ...) world->components[idx] = SparseSet_Alloc(sizeof(type), world->maxEntities);
    SOL_COMPONENT_LIST(INIT_COMPONENT)
#undef INIT_COMPONENT
}

void Sol_Comp_FreeAll(World *world)
{
    for (int i = 0; i < COMPONENT_COUNT; ++i)
    {
        BaseSparseSet *set = (BaseSparseSet *)world->components[i];
        if (set)
        {
            if (set->sparse)
                free(set->sparse);
            if (set->dense)
                free(set->dense);
            if (set->data)
                free(set->data);
            free(set);
            world->components[i] = NULL;
        }
    }
}

void World_InitSingletons(World *world)
{
#define SINGLETON_INIT(Type, InitFn, DeinitFn)                                                                         \
    {                                                                                                                  \
        Type *self = Sol_Comp_Add(world, 0, Type);                                                                     \
        if (self)                                                                                                      \
            InitFn(world, self);                                                                                       \
    }
    SOL_SINGLETON_LIST(SINGLETON_INIT)
#undef SINGLETON_INIT
}

void World_DeinitSingletons(World *world)
{
#define SINGLETON_DEINIT(Type, InitFn, DeinitFn)                                                                       \
    {                                                                                                                  \
        Type *self = Sol_Comp_Get(world, 0, Type);                                                                     \
        if (self)                                                                                                      \
            DeinitFn(self);                                                                                            \
    }
    SOL_SINGLETON_LIST(SINGLETON_DEINIT)
#undef SINGLETON_DEINIT
}

bool Sol_Comp_HasE(World *world, int id, u64 idx)
{
    return world->masks[id] & (1ULL << idx);
}

void *Sol_Comp_GetE(World *w, int id, u64 idx)
{
    if ((u32)id >= (u32)w->maxEntities || (u32)idx >= COMPONENT_COUNT || !(w->masks[id] & (1ULL << idx)))
        return NULL;

    BaseSparseSet *set = (BaseSparseSet *)w->components[idx];
    return ((char *)set->data) + (set->sparse[id] * set->data_size);
}

// static const void *component_defaults[COMPONENT_COUNT] = {
// #define REGISTER_COMPONENT_BASES(type, flag, base) [flag] = base,
//     SOL_COMPONENT_LIST(REGISTER_COMPONENT_BASES)
// #undef REGISTER_COMPONENT_BASES
// };

static const ComponentInitFn component_inits[COMPONENT_COUNT] = {
#define REGISTER_COMPONENT_ADDS(Type, Flag, AddFn) [Flag] = AddFn,
    SOL_COMPONENT_LIST(REGISTER_COMPONENT_ADDS)
#undef REGISTER_COMPONENT_ADDS
};

void *Sol_Comp_AddE(World *w, int entId, u64 idx)
{
    if ((uint32_t)entId >= w->maxEntities)
        return NULL;

    BaseSparseSet *set = (BaseSparseSet *)w->components[idx];
    size_t comp_size   = set->data_size;

    // 1. Check if entity already has component
    if (w->masks[entId] & (1ULL << idx))
    {
        return ((char *)set->data) + (set->sparse[entId] * comp_size);
    }

    // 2. Grow backing buffers if full
    if (set->cnt >= set->cap)
    {
        set->cap   = (set->cap == 0) ? 1 : set->cap * 2;
        set->dense = (int *)realloc(set->dense, set->cap * sizeof(int));
        set->data  = (void *)realloc(set->data, set->cap * comp_size);
    }

    // 3. Assign sparse and dense indices
    int denseIdx         = set->cnt++;
    set->sparse[entId]   = denseIdx;
    set->dense[denseIdx] = entId;

    // 4. Calculate byte pointer and initialize
    void *elem_ptr = ((char *)set->data) + (denseIdx * comp_size);
    memset(elem_ptr, 0, comp_size);
    if (component_inits[idx])
        component_inits[idx](w, entId, elem_ptr);
    // const void *defaults = component_defaults[idx];
    // if (defaults)
    //     memcpy(elem_ptr, defaults, comp_size);
    // else
    //     memset(elem_ptr, 0, comp_size);

    w->masks[entId] |= (1ULL << idx);

    return elem_ptr;
}

void Sol_Comp_RemE(World *w, int entId, u64 idx)
{
    if (!(w->masks[entId] & (1ULL << idx)))
        return;

    BaseSparseSet *set = (BaseSparseSet *)w->components[idx];
    int removedDense   = set->sparse[entId];
    int lastIdx        = set->cnt - 1;
    int lastEntity     = set->dense[lastIdx];
    size_t dataSize    = set->data_size;

    // Swap payload data in the dense array
    if (removedDense != lastIdx && set->data)
    {
        char *bytes = (char *)set->data;
        memcpy(bytes + (removedDense * dataSize), bytes + (lastIdx * dataSize), dataSize);
    }

    // Update dense/sparse indices
    set->dense[removedDense] = lastEntity;
    set->sparse[lastEntity]  = removedDense;

    set->cnt--;
    w->masks[entId] &= ~((1ULL << idx));
}
