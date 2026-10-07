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
int Sol_Weapon_Spawn(World *world, int owner, WeaponKind kind, const char *bone);
vec3s Sol_Weapon_DmgPos(World *world, int id);