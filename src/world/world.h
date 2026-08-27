#pragma once
#include "world_components.h"

#define WAdd2d(w) ((w)->draw2dSystems[(w)->draw2dCount++])
#define WAdd3d(w) ((w)->draw3dSystems[(w)->draw3dCount++])
#define WAddPrestep(w) ((w)->prestepSystems[(w)->prestepCount++])
#define WAddStep(w) ((w)->stepSystems[(w)->stepCount++])
#define WAddPoststep(w) (w->poststepSystems[w->poststepCount++])
#define WAddTick(w) ((w)->tickSystems[(w)->tickCount++])

// Forward declaration of World struct
typedef struct World World;

// ==========================================
// 3. GENERIC SPARSE SET STRUCT DECLARATOR
// ==========================================
typedef struct BaseSparseSet
{
    int   cnt;
    int   cap;
    int  *sparse;
    int  *dense;
    void *data;
} BaseSparseSet;
#define SPARSE_SET_STRUCT(T)                                                                                           \
    typedef struct SparseSet_##T                                                                                       \
    {                                                                                                                  \
        int  cnt;                                                                                                      \
        int  cap;                                                                                                      \
        int *sparse;                                                                                                   \
        int *dense;                                                                                                    \
        T   *data;                                                                                                     \
    } SparseSet_##T

// Declare all SparseSet structs
#define DECLARE_SPARSE_STRUCTS(type, flag) SPARSE_SET_STRUCT(type);
SOL_COMPONENT_LIST(DECLARE_SPARSE_STRUCTS)
#undef DECLARE_SPARSE_STRUCTS

// ==========================================
// 4. WORLD CONTAINER DEFINITION
// ==========================================

struct World
{
    SystemUpdate prestepSystems[MAX_SYSTEMS];
    SystemUpdate stepSystems[MAX_SYSTEMS];
    SystemUpdate poststepSystems[MAX_SYSTEMS];
    SystemUpdate tickSystems[MAX_SYSTEMS];
    SystemUpdate draw3dSystems[MAX_SYSTEMS];
    SystemUpdate draw2dSystems[MAX_SYSTEMS];

    u64   masks[MAX_ENTITIES];
    void *components[COMPONENT_COUNT];

    u32 hitGenMatrix[MAX_ENTITIES][MAX_ENTITIES];
    u32 globalHitGen;

    int prestepCount;
    int stepCount;
    int poststepCount;
    int tickCount;
    int draw2dCount;
    int draw3dCount;
    int deinitCount;

    u32  currentTick;
    int  maxEntities;
    bool doesSimulate, doesRender, doesReplicate;
};

// ==========================================
// 5. TYPED SPARSE SET IMPLEMENTATION GENERATOR
// ==========================================

#define GENERATE_SPARSE_SET_IMPL(T, ENUM_FLAG)                                                                         \
                                                                                                                       \
    static inline SparseSet_##T *Sol_SparseSet_Alloc_##T(int maxEnts)                                                  \
    {                                                                                                                  \
        SparseSet_##T *set = calloc(1, sizeof(SparseSet_##T));                                                         \
        set->cap           = INITIAL_SPARSE_SET_CAP;                                                                   \
        set->cnt           = 0;                                                                                        \
        set->sparse        = malloc(maxEnts * sizeof(int));                                                            \
        return (SparseSet_##T *)set;                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    static inline T *Sol_Comp_Add_##T(World *w, int entId)                                                             \
    {                                                                                                                  \
        SparseSet_##T *set = (SparseSet_##T *)w->components[ENUM_FLAG];                                                \
        if (w->masks[entId] & BITC(ENUM_FLAG))                                                                         \
        {                                                                                                              \
            return &set->data[set->sparse[entId]];                                                                     \
        }                                                                                                              \
        if (set->cnt >= set->cap)                                                                                      \
        {                                                                                                              \
            set->cap   = (set->cap == 0) ? 2 : set->cap * 2;                                                           \
            set->dense = realloc(set->dense, set->cap * sizeof(int));                                                  \
            set->data  = realloc(set->data, set->cap * sizeof(T));                                                     \
        }                                                                                                              \
        int denseIdx         = set->cnt++;                                                                             \
        set->sparse[entId]   = denseIdx;                                                                               \
        set->dense[denseIdx] = entId;                                                                                  \
        set->data[denseIdx]  = (T){0};                                                                                 \
        w->masks[entId] |= BITC(ENUM_FLAG);                                                                            \
        return &set->data[denseIdx];                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    static inline void Sol_Comp_Rem_##T(World *w, int entId)                                                           \
    {                                                                                                                  \
        if (!(w->masks[entId] & BITC(ENUM_FLAG)))                                                                      \
            return;                                                                                                    \
        SparseSet_##T *set          = (SparseSet_##T *)w->components[ENUM_FLAG];                                       \
        int            removedDense = set->sparse[entId];                                                              \
        int            lastEntity   = set->dense[set->cnt - 1];                                                        \
        set->data[removedDense]     = set->data[set->cnt - 1];                                                         \
        set->dense[removedDense]    = lastEntity;                                                                      \
        set->sparse[lastEntity]     = removedDense;                                                                    \
        set->cnt--;                                                                                                    \
        w->masks[entId] &= ~BITC(ENUM_FLAG);                                                                           \
    }

// Generate all typed inline Add/Rem/Alloc functions
#define GENERATE_SPARSE_FUNCS(type, flag) GENERATE_SPARSE_SET_IMPL(type, flag)
SOL_COMPONENT_LIST(GENERATE_SPARSE_FUNCS)
#undef GENERATE_SPARSE_FUNCS

// ==========================================
// 6. PUBLIC API ACCESSOR MACROS
// ==========================================

// Bitmask check: O(1), cache-friendly
#define Sol_Comp_Has(w, entId, Type) (((w)->masks[entId] & BITC(HAS_##Type)) != 0)

// Direct lookup via sparse index
#define Sol_Comp_Get(w, entId, Type)                                                                                   \
    (&((SparseSet_##Type *)(w)->components[HAS_##Type])                                                                \
          ->data[((SparseSet_##Type *)(w)->components[HAS_##Type])->sparse[entId]])

// Add component to entity (grows dense arrays dynamically if full)
#define Sol_Comp_Add(w, entId, Type) Sol_Comp_Add_##Type(w, entId)

// O(1) swap-with-back removal
#define Sol_Comp_Rem(w, entId, Type) Sol_Comp_Rem_##Type(w, entId)

// Access backing SparseSet directly for system dense iteration
#define Sol_Comp_Set(w, Type) ((SparseSet_##Type *)(w)->components[HAS_##Type])

// Initialize all sparse sets on the world
static inline void Sol_World_InitAllComponents(World *w, int maxEntities)
{
    w->maxEntities = maxEntities;
#define ALLOC_SPARSE_SET(type, flag) w->components[flag] = Sol_SparseSet_Alloc_##type(maxEntities);
    SOL_COMPONENT_LIST(ALLOC_SPARSE_SET)
#undef ALLOC_SPARSE_SET
}

static const size_t COMP_SIZES[COMPONENT_COUNT] = {
#define X(enum_name, type_name) sizeof(type_name),
    SOL_COMPONENT_LIST(X)
#undef X
};

static inline void Sol_Comp_RemE(World *w, int entId, int compEnum)
{
    if (!(w->masks[entId] & BITC(compEnum)))
        return;

    BaseSparseSet *set          = (BaseSparseSet *)w->components[compEnum];
    int            removedDense = set->sparse[entId];
    int            lastIdx      = set->cnt - 1;
    int            lastEntity   = set->dense[lastIdx];
    size_t         elemSize     = COMP_SIZES[compEnum];

    // Swap payload data in the dense array
    if (removedDense != lastIdx && set->data)
    {
        char *bytes = (char *)set->data;
        memcpy(bytes + (removedDense * elemSize), bytes + (lastIdx * elemSize), elemSize);
    }

    // Update dense/sparse indices
    set->dense[removedDense] = lastEntity;
    set->sparse[lastEntity]  = removedDense;

    set->cnt--;
    w->masks[entId] &= ~BITC(compEnum);
}

static inline void Sol_Destroy_Ent(World *w, int entId)
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
}

// Internal
void Worlds_Tick(World **worlds, int count, double dt, double time);
void Worlds_Step(World **worlds, int count, double dt, double time);
void Worlds_Draw3d(World **worlds, int count, double dt, double time);
void Worlds_Draw2d(World **worlds, int count, double dt, double time);

void Worlds_Xform_Snapshot(World **worlds, int count);
void Worlds_Xform_Interpolate(World **worlds, int count, float alpha);

void Model_Draw(World *world, double dt, double time);
void Anim_Tick(World *world, double dt, double time);

// Api
World *World_Create();
int    Sol_Create_Ent(World *world);
int    Sol_Create_EntNoXform(World *world);
void   Sol_Destroy_Ent(World *world, int id);

SolXform *Sol_Xform_Add(World *world, int id, vec3s pos);
SolAnim  *Sol_Anim_Add(World *world, int id);

void Sol_Anim_Play(World *world, int id, AnimDesc desc);
void Sol_Anim_Stop(World *world, int id, AnimLayerId layerId, float blendOut);
void Sol_Anim_SetSpeed(World *world, int id, AnimLayerId layerId, float rate);
void Sol_Anim_SetSeek(World *world, int id, AnimLayerId layerId, float seek);

void Sol_Xform_Teleport(World *world, int id, vec3s pos);
bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);