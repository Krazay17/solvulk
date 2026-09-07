/*
 * File: world.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-26
 *
 */

#pragma once
#include "components.h"

#define WAddTick(w) ((w)->tickSystems[(w)->tickCount++])
#define WAddStep(w) ((w)->stepSystems[(w)->stepCount++])
#define WAddPosttick(w) (w->posttickSystems[w->posttickCount++])
#define WAdd3d(w) ((w)->draw3dSystems[(w)->draw3dCount++])
#define WAdd2d(w) ((w)->draw2dSystems[(w)->draw2dCount++])

// Forward declaration of World struct
typedef struct World World;

typedef enum
{
    WORLDSYS_PLAYER,
    WORLDSYS_INTERACT,

    WORLDSYS_MOVE3,
    WORLDSYS_MOVE2,
    WORLDSYS_BODY3,
    WORLDSYS_BODY2,
    WORLDSYS_ABILITY,
    WORLDSYS_COMBAT,
    WORLDSYS_HOOK,

    WORLDSYS_FACING,
    WORLDSYS_CAMERA,
    WORLDSYS_ANIM,
    WORLDSYS_MODEL,
    WORLDSYS_VIEW2,
    WORLDSYS_SCOREBOARD,

    WORLDSYS_DEBUG,
    WORLDSYS_COUNT,
} WorldSystems;

// ==========================================
// 3. GENERIC SPARSE SET STRUCT DECLARATOR
// ==========================================
typedef struct BaseSparseSet
{
    int cnt;
    int cap;
    int *sparse;
    int *dense;
    void *data;
} BaseSparseSet;
#define SPARSE_SET_STRUCT(T)                                                                                           \
    typedef struct SparseSet_##T                                                                                       \
    {                                                                                                                  \
        int cnt;                                                                                                       \
        int cap;                                                                                                       \
        int *sparse;                                                                                                   \
        int *dense;                                                                                                    \
        T *data;                                                                                                       \
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
    SystemUpdate tickSystems[MAX_SYSTEMS];
    SystemUpdate stepSystems[MAX_SYSTEMS];
    SystemUpdate posttickSystems[MAX_SYSTEMS];
    SystemUpdate draw3dSystems[MAX_SYSTEMS];
    SystemUpdate draw2dSystems[MAX_SYSTEMS];

    u64 masks[MAX_ENTS];
    void *components[COMPONENT_COUNT];

    u64 system_mask;
    void *systems[WORLDSYS_COUNT];

    u32 hitGenMatrix[MAX_ENTS][MAX_ENTS];
    u32 globalHitGen;

    int tickCount;
    int stepCount;
    int posttickCount;
    int draw3dCount;
    int draw2dCount;

    int activeEnts[MAX_ENTS];
    int entCount;

    u32 currentTick, currentStep;
    double tickTime, stepTime;
    int maxEntities;
    int index;
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
        set->cap           = 0;                                                                                        \
        set->cnt           = 0;                                                                                        \
        set->sparse        = malloc(maxEnts * sizeof(int));                                                            \
        return (SparseSet_##T *)set;                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    static inline T *Sol_Comp_Add_##T(World *w, int entId)                                                             \
    {                                                                                                                  \
        if ((uint32_t)entId >= w->maxEntities)                                                                         \
            return NULL;                                                                                               \
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
        return (T *)&set->data[denseIdx];                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    static inline void Sol_Comp_Rem_##T(World *w, int entId)                                                           \
    {                                                                                                                  \
        if (!(w->masks[entId] & BITC(ENUM_FLAG)))                                                                      \
            return;                                                                                                    \
        SparseSet_##T *set       = (SparseSet_##T *)w->components[ENUM_FLAG];                                          \
        int removedDense         = set->sparse[entId];                                                                 \
        int lastEntity           = set->dense[set->cnt - 1];                                                           \
        set->data[removedDense]  = set->data[set->cnt - 1];                                                            \
        set->dense[removedDense] = lastEntity;                                                                         \
        set->sparse[lastEntity]  = removedDense;                                                                       \
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
#define Sol_Comp_Has(w, entId, Type) ((u32)(entId) < (w)->maxEntities && (((w)->masks[entId] & BITC(HAS_##Type)) != 0))

// Direct lookup via sparse index
#define Sol_Comp_Get(w, entId, Type)                                                                                   \
    (Sol_Comp_Has((w), (entId), Type)                                                                                  \
         ? (&((SparseSet_##Type *)(w)->components[HAS_##Type])                                                         \
                 ->data[((SparseSet_##Type *)(w)->components[HAS_##Type])->sparse[(entId)]])                           \
         : NULL)

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
#define X(type_name, enum_name) sizeof(type_name),
    SOL_COMPONENT_LIST(X)
#undef X
};

static inline void Sol_Comp_RemE(World *w, int entId, int compEnum)
{
    if (!(w->masks[entId] & BITC(compEnum)))
        return;

    BaseSparseSet *set = (BaseSparseSet *)w->components[compEnum];
    int removedDense   = set->sparse[entId];
    int lastIdx        = set->cnt - 1;
    int lastEntity     = set->dense[lastIdx];
    size_t elemSize    = COMP_SIZES[compEnum];

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

// Free all sparse set component arrays and their container memory
static inline void Sol_World_FreeAllComponents(World *w)
{
    for (int i = 0; i < COMPONENT_COUNT; ++i)
    {
        BaseSparseSet *set = (BaseSparseSet *)w->components[i];
        if (set)
        {
            if (set->sparse)
                free(set->sparse);
            if (set->dense)
                free(set->dense);
            if (set->data)
                free(set->data);
            free(set);
            w->components[i] = NULL;
        }
    }
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
    w->entCount--;
    w->activeEnts[entId] = false;
}

// Internal
void Worlds_Tick(World **worlds, int count, double dt);
void Worlds_Step(World **worlds, int count, double dt);
void Worlds_Draw3d(World **worlds, int count, double dt);
void Worlds_Draw2d(World **worlds, int count, double dt);
void Worlds_PostTick(World **worlds, int count, double dt);

void Worlds_Xform_Snapshot(World **worlds, int count);
void Worlds_Xform_Interpolate(World **worlds, int count, float alpha);

// Systems
void Combat_Init(World *world);
void Player_Init(World *world);
void Player_Deinit(World *world);
void Move3_Init(World *world);
void Move3_Deinit(World *world);
void Physx_Init(World *world);
void Physx_Deinit(World *world);
void Anim_Init(World *world);
void Anim_Deinit(World *world);
void Camera_Init(World *world);
void Camera_Deinit(World *world);
void Model_Init(World *world);
void Model_Deinit(World *world);
void Debug_Init(World *world);
void Debug_Deinit(World *world);

void Player_Tick(World *world, double dt);
void Interact_Tick(World *world, double dt);

void Move3_Step(World *world, double dt);
void Move2_Step(World *world, double dt);
void Body3_Step(World *world, double dt);
void Body2_Step(World *world, double dt);
void Ability_Step(World *world, double dt);
void Combat_Step(World *world, double dt);

void Hook_Tick(World *world, double dt);
void Anim_Tick(World *world, double dt);
void Facing_Tick(World *world, double dt);
void Camera_Tick(World *world, double dt);

void Scoreboard_Draw(World *world, double dt);
void Model_Render(World *world, double dt);
void Ability_Draw(World *world, double dt);
void View2_Draw(World *world, double dt);
void View2_Healthbar(World *world, double dt);
void View2_Abilitybar(World *world, double dt);
void Debug_Tick(World *world, double dt);
void Debug_Draw3(World *world, double dt);
void Debug_Draw2(World *world, double dt);

// Api
World *World_Create();
World *World_Create_AllSys();
int Sol_Create_Ent(World *world);
void Sol_Sys_Add(World *world, WorldSystems system);
void Sol_Sys_Remove(World *world, WorldSystems system);

void Sol_Xform_Teleport(World *world, int id, vec3s pos);

int Sol_Interact_FindTopmost(World *world, vec2s point);

void Sol_Anim_Play(World *world, int id, AnimDesc desc);
void Sol_Anim_Stop(World *world, int id, AnimLayerId layerId, float blendOut);
void Sol_Anim_SetSpeed(World *world, int id, AnimLayerId layerId, float rate);
void Sol_Anim_SetSeek(World *world, int id, AnimLayerId layerId, float seek);

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);

bool Sol_Move3_SetState(World *world, int id, MoveState state);
float Sol_Move3_GetBaseSpeed(World *world, int id);

bool Sol_Body3_DoesCollide(ScBody3 *body, ScBody3 *other_body);
vec3s Sol_Body3_GetGround(World *world, int id);
vec3s Sol_Body3_GetVel(World *world, int id);
vec3s Sol_Body3_GetDir(World *world, int id);
float Sol_Body3_GetSpeed(World *world, int id);

int Sol_Body2_GetEntAtPoint(World *world, vec2s point);
bool Sol_Body2_ContainsPoint(World *world, int id, vec2s point);

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force);

int Sol_Raycast(World *world, SolRay ray, SolRayResult *result, int max);
int Sol_RaycastD(World *world, SolRay ray, SolRayResult *result, int max, float time);
bool Sol_Raycast1(World *world, SolRay ray, SolRayResult *outResult);
bool Sol_Raycast1D(World *world, SolRay ray, SolRayResult *result, float time);

SolLine *Sol_Debug_NewLine(World *world, float ttl);
SolSphere *Sol_Debug_NewSphere(World *world, float ttl);

float Sol_Combat_Hit(World *world, int id, SolHit hit);
float Sol_Combat_Damage(World *world, int id, ScCombat *combat, float amount);
float Sol_Combat_Heal(World *world, int id, ScCombat *combat, float amount);
