/*
 * File: world.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-26
 *
 */
#pragma once
#include "sol/types.h"
#include "components.h"
#include "configs.h"

#define MAX_SYSTEMS 64

#define WAddTick(w) ((w)->tickSystems[(w)->tickCount++])
#define WAddStep(w) ((w)->stepSystems[(w)->stepCount++])
#define WAddPosttick(w) (w->posttickSystems[w->posttickCount++])
#define WAdd3d(w) ((w)->draw3dSystems[(w)->draw3dCount++])
#define WAdd2d(w) ((w)->draw2dSystems[(w)->draw2dCount++])

typedef struct World World;

typedef enum
{
    PHASE_PRE,
    PHASE_STEP,
    PHASE_POST,
    PHASE_RENDER3,
    PHASE_RENDER2,
} Phase;

#define SOL_SYSTEMS(X)                                                                                                 \
    X(SYS_TEST, PHASE_PRE, Sol_Test)                                                                                   \
    X(SYS_TIMER, PHASE_PRE, Timer_Update)                                                                              \
    X(SYS_CMD, PHASE_PRE, Cmd_Update)                                                                                  \
    X(SYS_PLAYER, PHASE_PRE, Player_Update)                                                                            \
    X(SYS_INTERACT, PHASE_PRE, Interact_Update)                                                                        \
    X(SYS_FACING, PHASE_PRE, Facing_Tick)                                                                              \
    X(SYS_DEBUG, PHASE_PRE, Debug_Tick)                                                                                \
                                                                                                                       \
    X(SYS_DRAG, PHASE_STEP, Drag_Update)                                                                               \
    X(SYS_ITEM, PHASE_STEP, Item_Update)                                                                               \
    X(SYS_BUFF, PHASE_STEP, Buff_Update)                                                                               \
    X(SYS_MOVE3, PHASE_STEP, Move3_Update)                                                                             \
    X(SYS_MOVE2, PHASE_STEP, Move2_Update)                                                                             \
    X(SYS_BODY3, PHASE_STEP, Body3_Update)                                                                             \
    X(SYS_BODY2, PHASE_STEP, Body2_Step)                                                                               \
    X(SYS_ABILITY, PHASE_STEP, Ability_Update)                                                                         \
    X(SYS_PROJECTILE, PHASE_STEP, Projectile_Step)                                                                     \
    X(SYS_ZONE, PHASE_STEP, Zone_Update)                                                                               \
    X(SYS_COMBAT, PHASE_STEP, Combat_Update)                                                                           \
    X(SYS_PICKUP, PHASE_STEP, Pickup_Update)                                                                           \
    X(SYS_AI, PHASE_STEP, Ai_Step)                                                                                     \
    X(SYS_SCORE, PHASE_STEP, Score_Update)                                                                             \
    X(SYS_RULES, PHASE_STEP, Rules_Update)                                                                             \
    X(SYS_AUDIO, PHASE_STEP, Audio_Update)                                                                             \
    X(SYS_FX, PHASE_STEP, Fx_Update)                                                                                   \
    X(SYS_EVENT, PHASE_STEP, Event_Step)                                                                               \
    X(SYS_EVENTCLEAR, PHASE_STEP, Event_Clear)                                                                         \
                                                                                                                       \
    X(SYS_UI, PHASE_POST, Ui_Update)                                                                                   \
    X(SYS_HOOK, PHASE_POST, Hook_Tick)                                                                                 \
    X(SYS_CAMERA, PHASE_POST, Camera_Tick)                                                                             \
    X(SYS_ANIM, PHASE_POST, Anim_Tick)                                                                                 \
    X(SYS_PARENT, PHASE_POST, Parent_Update)                                                                           \
    X(SYS_WEAPON, PHASE_POST, Weapon_Update)                                                                           \
    X(SYS_EMITTER, PHASE_POST, Emitter_Update)                                                                         \
                                                                                                                       \
    X(SYS_BUFF_DRAW, PHASE_RENDER3, Buff_Draw)                                                                         \
    X(SYS_ABILITY_DRAW, PHASE_RENDER3, Ability_Draw)                                                                   \
    X(SYS_PARTICLE_DRAW, PHASE_RENDER3, Particle_Draw)                                                                 \
    X(SYS_RIBBON_DRAW, PHASE_RENDER3, Ribbon_Update)                                                                   \
    X(SYS_MODEL_DRAW, PHASE_RENDER3, Model_Render)                                                                     \
    X(SYS_VIEW3_DRAW, PHASE_RENDER3, View3_Draw)                                                                       \
    X(SYS_DEBUG_DRAW3, PHASE_RENDER3, Debug_Draw3)                                                                     \
                                                                                                                       \
    X(SYS_SCOREBOARD_DRAW, PHASE_RENDER2, Scoreboard_Draw)                                                             \
    X(SYS_RULES_DRAW, PHASE_RENDER2, Rules_Draw)                                                                       \
    X(SYS_VIEW2_DRAW, PHASE_RENDER2, View2_Draw)                                                                       \
    X(SYS_DEBUG_DRAW2, PHASE_RENDER2, Debug_Draw2)

typedef enum
{
#define AS_ENUM(ENUM, ...) ENUM,
    SOL_SYSTEMS(AS_ENUM)
#undef AS_ENUM
    SYS_COUNT,
} SolSys;

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

typedef enum
{
    ONSKIND_DESTROY,
    ONSKIND_COUNT,
} OnsKind;
struct World
{
    UpdateFn tickSystems[MAX_SYSTEMS];
    UpdateFn stepSystems[MAX_SYSTEMS];
    UpdateFn posttickSystems[MAX_SYSTEMS];
    UpdateFn draw3dSystems[MAX_SYSTEMS];
    UpdateFn draw2dSystems[MAX_SYSTEMS];

    WorldXform xform;

    u64 masks[MAX_ENTS];
    u64 system_mask;
    void *components[COMPONENT_COUNT];

    // void (**ons[ONSKIND_COUNT])(World *world);

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

void Sol_Sys_Add(World *world, SolSys system);
void Sol_Sys_Remove(World *world, SolSys system);
void Sol_Sys_Add_Ui(World *world);

void Sol_Xform_Teleport(World *world, int id, vec3s pos);

int Sol_Interact_FindTopmost(World *world, vec2s point);

Xform Sol_Model_GetBoneXform(World *world, int id, const char *name);

void Sol_Anim_Play(World *world, int id, AnimDesc desc);
void Sol_Anim_Stop(World *world, int id, AnimLayerId layerId, float blendOut);
void Sol_Anim_SetSpeed(World *world, int id, AnimLayerId layerId, float rate);
void Sol_Anim_SetSeek(World *world, int id, AnimLayerId layerId, float seek);

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);

AbilityConfig Sol_Ability_GetSlotConf(World *world, int id, int slot);

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

float Sol_Combat_Hit(World *world, SolHit hit);
float Sol_Combat_Damage(World *world, int target, int dealer, float amount);
float Sol_Combat_Heal(World *world, int target, int dealer, float amount);

void Sol_Emitter_Spawn(World *world, EmitterKind kind, vec3s pos);
void Sol_Emitter_Push(World *world, Emitter *emitters, int count);
void Sol_Emitter_PushE(World *world, Emitter *emitters, int count, vec3s pos, vec3s dir, float speed);
Emitter *Sol_Emitter_Next(World *world, EmitterKind kind);

u32 Sol_Hitgen_Start(World *world);
bool Sol_Hitgen_Has(World *world, int id, int target, u32 sessionGen);
bool Sol_Hitgen_Try(World *world, int id, int target, u32 sessionGen);

void Sol_Event_Push(World *world, EventKind kind, SolEvent event);
void Sol_Event_AddHandler(World *world, EventKind kind, EventHandler handler);

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
float Sol_Ability_GetCurrentBaseDuration(World *world, int id, int slot);
AbilityConfig Sol_Item_ApplyMods(AbilityConfig conf, SolItem item);
AbilityConfig Sol_Item_GetMods(SolItem item);
DefendResult Sol_Ability_TryDefend(World *world, int id, SolHit *hit);
void Sol_Combat_Chain(World *world, ChainhitKind kind, SolHit hit, float radius, float rate, int chain_count);
void Sol_Combat_Reflect(World *world, int projectile, int reflector, vec3s pos);
u32 Sol_Combat_ClosestTargetLos(World *world, int id, SolRay ray, int hitgen);
SolShoot Sol_Combat_GetShoot(World *world, int id, float fwd_offset);
float Sol_GetBoneRoll(vec3s tangent, vec3s boneUp);
vec3s Sol_Weapon_DmgPos(World *world, int id);
Xform Sol_Weapon_BladeXform(World *world, int id);
void Sol_Ability_Equip(World *world, int id, int slot, SolItem *item);
void Sol_Ability_SetAllIdle(World *world, int id);
bool Sol_CanAbility(World *world, int id);
bool Sol_CanMove(World *world, int id);
bool Sol_Combat_UseEnergy(World *world, int id, float amount);
void Sol_Combat_AddEnergy(World *world, int id, float amount);
bool Sol_Ability_IsCharging(World *world, int id);
bool Sol_Ability_AnyActive(World *world, int id);
void Sol_Cmd_Clear(World *world, int id);
void Sol_Weapon_Extend(World *world, int id, float power);
void Sol_Weapon_Unextend(World *world, int id);
int Sol_Parent_GetChildren(World *world, int id, int *ids, int max_ids);
// void Item_User_Update(World *world, SolItem *items, int count, u32 kind);
SolItem *Sol_Item_Resolve(World *world, ScRef *ref);
void Sol_Tooltip_Draw(World *world, int id, UserInput *input, float alpha);
void Sol_Ui_User_Update(World *hud, World *game, int user_ent);
void Sol_Interact_Post(World *w, int id, u32 bits);
void Sol_Interact_BeginDrag(World *w, int id, vec3s point);
void Sol_Interact_Drag(World *w, int id, vec3s point);
void Sol_Interact_Down(World *w, int id, vec3s point);
void Sol_Inv_Equip(World *world, int id, int slot, int idx);
SolItem Sol_Inv_Remove(World *world, int id, u32 idx);
int Sol_Equip_Item(World *world, int id, u32 kind, int slot);
int Sol_Equip_GetId(World *world, int id, int slot);
void Sol_Weapon_TrailsOff(World *world, int id);
SolRay Sol_Weapon_Ray(World *world, int id, int right_hand, float power);
void Sol_Weapon_TrailOn(World *world, int id, bool is_right);
SolItem Sol_Inv_GetItemAtSlot(const ScInventory *inv, int slot);
SolItem *Sol_Inv_GetItem_FromId(ScInventory *inv, u32 uid);
void Sol_Ui_Sync_Inventory(World *hud, World *game, int user_ent, vec3s pos);
int Sol_Inv_AddItem(World *world, int id, SolItem *item, bool try_equip);
void Sol_Ui_Sync_AbilityItems(World *hud, World *game, int user_ent);
void Sol_Ui_CloseInv(World *hud, vec3s *inv_pos);
void Sol_Ui_Sync_Rules(World *hud, World *game, double dt);
void Sol_Inv_AllDirty(World *world);
vec3s Sol_Cmd_GetDir(World *world, int id);
vec3s Sol_Model_GetBoneByIdx(World *world, int id, u32 boneIdx);
int Sol_Weapon_Trace(World *world, int id, SolHit hit, u32 hitgen);
void Sol_Ai_RandomName(char *dest, size_t size);
