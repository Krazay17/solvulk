/*
 * File: component.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-04
 *
 */
#include "components.h"
#include "world.h"

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
#define INIT_COMPONENT(type, idx) world->components[idx] = SparseSet_Alloc(sizeof(type), world->maxEntities);
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
