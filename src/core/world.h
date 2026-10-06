/*
 * File: world.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-26
 *
 */
#pragma once
#include "component.h"
#include "configs.h"

#define MAX_SYSTEMS 64

#define WAddTick(w) ((w)->tickSystems[(w)->tickCount++])
#define WAddStep(w) ((w)->stepSystems[(w)->stepCount++])
#define WAddPosttick(w) (w->posttickSystems[w->posttickCount++])
#define WAdd3d(w) ((w)->draw3dSystems[(w)->draw3dCount++])
#define WAdd2d(w) ((w)->draw2dSystems[(w)->draw2dCount++])

// Forward declaration of World struct
typedef struct World World;

typedef enum
{
    UPDATEPHASE_TICK,
    UPDATEPHASE_STEP,
    UPDATEPHASE_POSTTICK,
    UPDATEPHASE_RENDER3,
    UPDATEPHASE_RENDER2,
} UpdatePhase;

typedef enum
{
    WORLDSYS_TEST,

    WORLDSYS_CMD,
    WORLDSYS_PLAYER,
    WORLDSYS_INTERACT,
    WORLDSYS_PARENT,

    WORLDSYS_BUFF,
    WORLDSYS_ABILITYBAR,
    WORLDSYS_MOVE3,
    WORLDSYS_MOVE2,
    WORLDSYS_BODY3,
    WORLDSYS_BODY2,
    WORLDSYS_ABILITY,
    WORLDSYS_PROJECTILE,
    WORLDSYS_ZONE,
    WORLDSYS_COMBAT,
    WORLDSYS_HOOK,
    WORLDSYS_AI,

    WORLDSYS_REF,
    WORLDSYS_FX,
    WORLDSYS_EMITTER,
    WORLDSYS_RIBBON,
    WORLDSYS_FACING,
    WORLDSYS_CAMERA,
    WORLDSYS_ANIM,
    WORLDSYS_MODEL,
    WORLDSYS_VIEW2,
    WORLDSYS_VIEW3,
    WORLDSYS_SCOREBOARD,
    WORLDSYS_TIMER,

    WORLDSYS_DEBUG,
    WORLDSYS_COUNT,
} WorldSystems;

// ==========================================
// 4. WORLD CONTAINER DEFINITION
// ==========================================
typedef struct WorldXform
{
    vec3s pos[MAX_ENTS];
    vec3s last_pos[MAX_ENTS];
    vec3s draw_pos[MAX_ENTS];
    vec3s sca[MAX_ENTS];
    vec3s last_sca[MAX_ENTS];
    vec3s draw_sca[MAX_ENTS];
    versors rot[MAX_ENTS];
    versors last_rot[MAX_ENTS];
    versors draw_rot[MAX_ENTS];
    vec3s home_pos[MAX_ENTS];
} WorldXform;

struct World
{
    SystemUpdate tickSystems[MAX_SYSTEMS];
    SystemUpdate stepSystems[MAX_SYSTEMS];
    SystemUpdate posttickSystems[MAX_SYSTEMS];
    SystemUpdate draw3dSystems[MAX_SYSTEMS];
    SystemUpdate draw2dSystems[MAX_SYSTEMS];

    WorldXform xform;

    u64 masks[MAX_ENTS];
    u64 system_mask;
    void *components[COMPONENT_COUNT];

    int tickCount;
    int stepCount;
    int posttickCount;
    int draw3dCount;
    int draw2dCount;

    int maxEntities;
    int entCount;
    int sparse[MAX_ENTS];
    int dense[MAX_ENTS];

    double dt;
    double tickTime, stepTime;
    float fdt;
    float timescale;
    u32 currentTick, currentStep;
    int index;
    bool doesSimulate, doesRender, doesReplicate;
};

// ==========================================
// 5. TYPED SPARSE SET IMPLEMENTATION GENERATOR
// ==========================================

// #define GENERATE_SPARSE_SET_IMPL(T, ENUM_FLAG)                                                                         \
//                                                                                                                        \
//     static inline SparseSet_##T *Sol_SparseSet_Alloc_##T(int maxEnts)                                                  \
//     {                                                                                                                  \
//         SparseSet_##T *set = (SparseSet_##T *)calloc(1, sizeof(SparseSet_##T));                                        \
//         set->cap           = 0;                                                                                        \
//         set->cnt           = 0;                                                                                        \
//         set->sparse        = (int *)malloc(maxEnts * sizeof(int));                                                     \
//         return (SparseSet_##T *)set;                                                                                   \
//     }                                                                                                                  \
//                                                                                                                        \
//     static inline T *Sol_Comp_Add_##T(World *w, int entId)                                                             \
//     {                                                                                                                  \
//         if ((uint32_t)entId >= w->maxEntities)                                                                         \
//             return NULL;                                                                                               \
//         SparseSet_##T *set = (SparseSet_##T *)w->components[ENUM_FLAG];                                                \
//         if (w->masks[entId] & BITC(ENUM_FLAG))                                                                         \
//         {                                                                                                              \
//             return &set->data[set->sparse[entId]];                                                                     \
//         }                                                                                                              \
//         if (set->cnt >= set->cap)                                                                                      \
//         {                                                                                                              \
//             set->cap   = (set->cap == 0) ? 1 : set->cap * 2;                                                           \
//             set->dense = (int *)realloc(set->dense, set->cap * sizeof(int));                                           \
//             set->data  = (T *)realloc(set->data, set->cap * sizeof(T));                                                \
//         }                                                                                                              \
//         int denseIdx         = set->cnt++;                                                                             \
//         set->sparse[entId]   = denseIdx;                                                                               \
//         set->dense[denseIdx] = entId;                                                                                  \
//         memset(&set->data[denseIdx], 0, sizeof(T));                                                                    \
//         w->masks[entId] |= BITC(ENUM_FLAG);                                                                            \
//         return (T *)&set->data[denseIdx];                                                                              \
//     }                                                                                                                  \
//                                                                                                                        \
//     static inline void Sol_Comp_Rem_##T(World *w, int entId)                                                           \
//     {                                                                                                                  \
//         if (!(w->masks[entId] & BITC(ENUM_FLAG)))                                                                      \
//             return;                                                                                                    \
//         SparseSet_##T *set       = (SparseSet_##T *)w->components[ENUM_FLAG];                                          \
//         int removedDense         = set->sparse[entId];                                                                 \
//         int lastEntity           = set->dense[set->cnt - 1];                                                           \
//         set->data[removedDense]  = set->data[set->cnt - 1];                                                            \
//         set->dense[removedDense] = lastEntity;                                                                         \
//         set->sparse[lastEntity]  = removedDense;                                                                       \
//         set->cnt--;                                                                                                    \
//         w->masks[entId] &= ~BITC(ENUM_FLAG);                                                                           \
//     }

// // Generate all typed inline Add/Rem/Alloc functions
// #define GENERATE_SPARSE_FUNCS(type, flag) GENERATE_SPARSE_SET_IMPL(type, flag)
// SOL_COMPONENT_LIST(GENERATE_SPARSE_FUNCS)
// #undef GENERATE_SPARSE_FUNCS

// ==========================================
// 6. PUBLIC API ACCESSOR MACROS
// ==========================================

// // Bitmask check: O(1), cache-friendly
// #define Sol_Comp_Has(w, entId, Type) ((u32)(entId) < (w)->maxEntities && (((w)->masks[entId] & BITC(HAS_##Type)) !=
// 0))

// // Direct lookup via sparse index
// #define Sol_Comp_Get(w, entId, Type) \
//     (Sol_Comp_Has((w), (entId), Type) \
//          ? (&((SparseSet_##Type *)(w)->components[HAS_##Type]) \
//                  ->data[((SparseSet_##Type *)(w)->components[HAS_##Type])->sparse[(entId)]]) \
//          : NULL)

// // Add component to entity (grows dense arrays dynamically if full)
// #define Sol_Comp_Add(w, entId, Type) Sol_Comp_Add_##Type(w, entId)

// // O(1) swap-with-back removal
// #define Sol_Comp_Rem(w, entId, Type) Sol_Comp_Rem_##Type(w, entId)

// // Access backing SparseSet directly for system dense iteration
// #define Sol_Comp_Set(w, Type) ((SparseSet_##Type *)(w)->components[HAS_##Type])

// #define Sol_Comp_HasE(w, entId, idx) ((u32)(entId) < (w)->maxEntities && (((w)->masks[entId] & BITC(idx)) != 0))
// #define Sol_Comp_GetE(w, entId, idx) \
//     (Sol_Comp_HasE((w), (entId), idx) ? (&((w)->components[idx])->data[((w)->components[idx])->sparse[(entId)]]) :
//     NULL)

// // Initialize all sparse sets on the world
// static inline void Sol_World_InitAllComponents(World *w, int maxEntities)
// {
//     w->maxEntities = maxEntities;
// #define ALLOC_SPARSE_SET(type, flag) w->components[flag] = Sol_SparseSet_Alloc_##type(maxEntities);
//     SOL_COMPONENT_LIST(ALLOC_SPARSE_SET)
// #undef ALLOC_SPARSE_SET
// }

// static const size_t COMP_SIZES[COMPONENT_COUNT] = {
// #define X(type_name, enum_name) sizeof(type_name),
//     SOL_COMPONENT_LIST(X)
// #undef X
// };

// static inline void *Sol_Comp_AddE(World *w, int entId, int enum_idx)
// {
//     if ((uint32_t)entId >= w->maxEntities)
//         return NULL;

//     BaseSparseSet *set = (BaseSparseSet *)w->components[enum_idx];
//     size_t comp_size   = COMP_SIZES[enum_idx];

//     // 1. Check if entity already has component
//     if (w->masks[entId] & BITC(enum_idx))
//     {
//         return ((char *)set->data) + (set->sparse[entId] * comp_size);
//     }

//     // 2. Grow backing buffers if full
//     if (set->cnt >= set->cap)
//     {
//         set->cap   = (set->cap == 0) ? 1 : set->cap * 2;
//         set->dense = (int *)realloc(set->dense, set->cap * sizeof(int));
//         set->data  = (void *)realloc(set->data, set->cap * comp_size);
//     }

//     // 3. Assign sparse and dense indices
//     int denseIdx         = set->cnt++;
//     set->sparse[entId]   = denseIdx;
//     set->dense[denseIdx] = entId;

//     w->masks[entId] |= BITC(enum_idx);

//     // 4. Calculate byte pointer and initialize
//     void *elem_ptr = ((char *)set->data) + (denseIdx * comp_size);
//     memset(elem_ptr, 0, comp_size);
//     return elem_ptr;
// }

// static inline void Sol_Comp_RemE(World *w, int entId, int compEnum)
// {
//     if (!(w->masks[entId] & BITC(compEnum)))
//         return;

//     BaseSparseSet *set = (BaseSparseSet *)w->components[compEnum];
//     int removedDense   = set->sparse[entId];
//     int lastIdx        = set->cnt - 1;
//     int lastEntity     = set->dense[lastIdx];
//     size_t elemSize    = COMP_SIZES[compEnum];

//     // Swap payload data in the dense array
//     if (removedDense != lastIdx && set->data)
//     {
//         char *bytes = (char *)set->data;
//         memcpy(bytes + (removedDense * elemSize), bytes + (lastIdx * elemSize), elemSize);
//     }

//     // Update dense/sparse indices
//     set->dense[removedDense] = lastEntity;
//     set->sparse[lastEntity]  = removedDense;

//     set->cnt--;
//     w->masks[entId] &= ~BITC(compEnum);
// }

// Free all sparse set component arrays and their container memory
// static inline void World_FreeAllComponents(World *w)
// {
//     for (int i = 0; i < COMPONENT_COUNT; ++i)
//     {
//         BaseSparseSet *set = (BaseSparseSet *)w->components[i];
//         if (set)
//         {
//             if (set->sparse)
//                 free(set->sparse);
//             if (set->dense)
//                 free(set->dense);
//             if (set->data)
//                 free(set->data);
//             free(set);
//             w->components[i] = NULL;
//         }
//     }
// }

// void *Sol_Comp_AddE(World *w, int entId, int comp_idx);
// void Sol_Comp_RemE(World *w, int entId, int comp_idx);

static inline Xform Xform_GetDraw(const World *world, int id)
{
    Xform xform;
    xform.pos = world->xform.draw_pos[id];
    xform.rot = world->xform.draw_rot[id];
    xform.sca = world->xform.draw_sca[id];
    return xform;
}

static inline Xform Xform_Get(const World *world, int id)
{
    Xform xform;
    xform.pos = world->xform.pos[id];
    xform.rot = world->xform.rot[id];
    xform.sca = world->xform.sca[id];
    return xform;
}

static inline XformP Xform_GetP(World *world, int id)
{
    WorldXform *x = &world->xform;
    XformP xformP;
    xformP.pos = &x->pos[id];
    xformP.rot = &x->rot[id];
    xformP.sca = &x->sca[id];
    return xformP;
}

static inline void Xform_SetAll(World *world, int id, vec3s pos, versors rot, vec3s sca)
{
    world->xform.pos[id]      = pos;
    world->xform.draw_pos[id] = pos;
    world->xform.last_pos[id] = pos;

    world->xform.rot[id]      = rot;
    world->xform.draw_rot[id] = rot;
    world->xform.last_rot[id] = rot;

    world->xform.sca[id]      = sca;
    world->xform.draw_sca[id] = sca;
    world->xform.last_sca[id] = sca;
}

static inline void Xform_SetSca(World *world, int id, vec3s sca)
{
    world->xform.sca[id]      = sca;
    world->xform.draw_sca[id] = sca;
    world->xform.last_sca[id] = sca;
}
static inline void Xform_SetRot(World *world, int id, versors rot)
{
    world->xform.rot[id]      = rot;
    world->xform.draw_rot[id] = rot;
    world->xform.last_rot[id] = rot;
}

void Worlds_Tick(World **worlds, int count, double dt);
void Worlds_Step(World **worlds, int count, double dt);
void Worlds_Draw3d(World **worlds, int count, double dt);
void Worlds_Draw2d(World **worlds, int count, double dt);
void Worlds_PostTick(World **worlds, int count, double dt);

void Worlds_Xform_Snapshot(World **worlds, int count);
void Worlds_Xform_Interpolate(World **worlds, int count, float alpha);
void Worlds_Event_Clear(World **worlds, int count);

// Api
World *World_Create();
World *World_Create_AllSys();
void World_Destroy(World *world);

int Sol_Create_Ent(World *world, vec3s pos);
void Sol_Destroy_Ent(World *w, int id);
int Sol_Duplicate_Ent(World *world, int id, World *target_world, vec3s pos);

void Sol_Sys_Add(World *world, WorldSystems system);
void Sol_Sys_Remove(World *world, WorldSystems system);
void Sol_Xform_Teleport(World *world, int id, vec3s pos);

int Sol_Interact_FindTopmost(World *world, vec2s point);

Xform Sol_Model_GetBoneXform(World *world, int id, const char *name);

void Sol_Anim_Play(World *world, int id, AnimDesc desc);
void Sol_Anim_Stop(World *world, int id, AnimLayerId layerId, float blendOut);
void Sol_Anim_SetSpeed(World *world, int id, AnimLayerId layerId, float rate);
void Sol_Anim_SetSeek(World *world, int id, AnimLayerId layerId, float seek);

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);

AbilityConfig Sol_Ability_GetSlotConf(const ScAbility *ability, int slot);

bool Sol_Move3_SetState(World *world, int id, MoveState state);
float Sol_Move3_GetBaseSpeed(World *world, int id);

vec3s Sol_Body3_GetGround(World *world, int id);
vec3s Sol_Body3_GetVel(World *world, int id);
vec3s Sol_Body3_GetDir(World *world, int id);
float Sol_Body3_GetSpeed(World *world, int id);
vec3s Sol_Body3_GetHead(World *world, int id);
int Sol_Raycast(World *world, SolRay ray, SolRayResult *result, int max);
int Sol_RaycastD(World *world, SolRay ray, SolRayResult *result, int max, float time);
bool Sol_Raycast1(World *world, SolRay ray, SolRayResult *outResult);
bool Sol_Raycast1D(World *world, SolRay ray, SolRayResult *result, float time);
int Sol_Spherecast(World *world, SolRay ray, SolRayResult *result, int max);
int Sol_SpherecastD(World *world, SolRay ray, SolRayResult *results, int max, float time);
int Sol_SphereOverlap(World *world, SolRay ray, SolRayResult *out_hits, int max_hits);
int Sol_SphereOverlapD(World *world, SolRay ray, SolRayResult *out_hits, int max_hits, float time);
bool Sol_CapsuleOverlap(World *world, vec3s a0, vec3s a1, vec3s b0, vec3s b1, float radiusA, float radiusB,
                        CapHit *out_hit);

int Sol_Body2_GetOverlaps(World *world, int id, int *ids, int max_counts);
int Sol_Body2_GetEntAtPoint(World *world, vec2s point);
bool Sol_Body2_ContainsPoint(World *world, int id, vec2s point);

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force);

SolLine *Sol_Debug_NewLine(World *world, float ttl);
SolSphere *Sol_Debug_NewSphere(World *world, float ttl);

float Sol_Combat_Hit(World *world, int id, SolHit hit);
float Sol_Combat_Damage(World *world, int id, int dealer, ScCombat *combat, float amount);
float Sol_Combat_Heal(World *world, int id, int dealer, ScCombat *combat, float amount);

void Sol_Emitter_Spawn(World *world, EmitterKind kind, vec3s pos);
void Sol_Emitter_Push(World *world, Emitter *emitters, int count);
void Sol_Emitter_PushE(World *world, Emitter *emitters, int count, vec3s pos, vec3s dir, float speed);
Emitter *Sol_Emitter_Next(World *world, EmitterKind kind);

u32 Sol_Hitgen_Start(World *world);
bool Sol_Hitgen_Has(World *world, int id, int target, u32 sessionGen);
bool Sol_Hitgen_Try(World *world, int id, int target, u32 sessionGen);
void Sol_Event_Push(World *world, EventKind kind, SolEvent event);

void Sol_Buff_Add(World *world, int id, BuffKind kind, u32 source, float power);
void Sol_Buff_AddMask(World *world, int id, u32 mask, u32 source, float power);
void Sol_Buff_AddE(World *world, int id, BuffKind kind, u32 source, float power, float duration);
Buff *Sol_Buff_Next(World *world, int id, BuffKind kind);
void Sol_Buff_Rem(World *world, int id, BuffKind kind);
void Sol_Ai_QuickLearn(World *world, int id, int ownerId, bool once);
void Sol_Combat_DamageSphere(World *world, int id, SolRay ray, SolHit hit, u32 hitgen);
int Sol_Combat_DamageCast(World *world, int id, SolRay ray, SolHit hit, u32 hitgen);
bool Sol_Ability_GetIsDashing(const ScAbility *ability);
bool Sol_Combat_Hostile(World *world, int idA, int idB);
float Sol_Ability_GetCurrentBaseDuration(const ScAbility *ability, int slot);
AbilityConfig Sol_Item_ApplyMods(AbilityConfig conf, SolItem item);
AbilityConfig Sol_Item_GetMods(SolItem item);
DefendResult Sol_Ability_TryDefend(World *world, int id, SolHit *hit);
void Sol_Combat_Chain(World *world, ChainhitKind kind, SolHit hit, float radius, float rate, int chain_count);
void Sol_Combat_Reflect(World *world, int projectile, int reflector, vec3s pos);
void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s posA, vec3s posB);
void Sol_Ribbon_SpawnE(World *world, RibbonKind kind, u32 entA, u32 entB);
Ribbon *Sol_Ribbon_Next(World *world);
bool Sol_Ribbon_Draw(World *world, double dt, Ribbon *r);
u32 Sol_Combat_ClosestTargetLos(World *world, int id, SolRay ray, int hitgen);
SolShoot Sol_Combat_GetShoot(World *world, int id, float fwd_offset);
void Sol_Ribbon_Addpoint(Ribbon *r, vec3s pos, float jitter_mag, float spin);
ScRibbon *Sol_Ribbon_AddKind(World *world, int id, RibbonKind kind);
void Sol_Ribbon_GenerateJitter(Ribbon *r, float jitter_mag);
float Sol_GetBoneRoll(vec3s tangent, vec3s boneUp);