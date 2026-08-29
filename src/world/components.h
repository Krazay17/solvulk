#pragma once

#include "sol/types.h"

#define MAX_ENTITIES 4096
#define MAX_SYSTEMS 64
#define MAX_BUFFS 64
#define MAX_VIEWS 10
#define MAX_TRACKER_GETTERS 2
#define MAX_EMITTERS 8
#define INITIAL_SPARSE_SET_CAP 0


// ==========================================
// 1. COMPONENT DATA STRUCTS
// ==========================================
typedef struct SolActive
{
    int    active_at_tick;
    double time_activated;
} SolActive;

typedef struct SolXform
{
    vec3s   last_pos, pos, draw_pos;
    vec3s   last_sca, sca, draw_sca;
    versors last_rot, rot, draw_rot;
} SolXform;

typedef struct SolController
{
    u8         kind;
    SolActions actionState;
    int        aimHitEnt;
    float      yaw, pitch;

    vec3s wishdir, wishdirY, aimdir, aimpos, lookdir, knockDur;
    vec2s wishdir2d, aimpos2d;

    bool isStrafing;
} SolController;

typedef struct SolBody3
{
    Shape3 shape;
    vec3s  vel, impulse, force, groundNormal, dims;
    vec3s  gravity;
    float  mass, invMass, restitution;
    u32    spatial_grid_offset, tri_count;
    u32    group, base_group;
    u32    ray_group, ray_base_group;
    bool   ignoreFriendly;
    bool putInTable;
} SolBody3;

typedef struct SolBody2
{
    Shape2 shape;
    vec2s  vel, dims, grav, grabPos;
    u32    group, zindex;
    u32    overlap_group;
} SolBody2;

typedef struct SolCamera
{
    vec3s pos, anchor;
    vec3s dir;
    vec3s up, right;
    float current_distance, current_offset;
    float target_distance, target_offset;
    float lerpspeed;
    float fov;
    float roll;
} SolCamera;

typedef struct SolInteract
{
    double        hover_start_time;
    double        unhover_start_time;
    double        press_start_time;
    InteractState state;
    double        pressedAccum;

    SolCallback onClick;
    SolCallback onHold;

    vec3s offset, targetPos;
} SolInteract;
typedef struct
{
    double lastEntered, lastExited;
    float  elapsed, accum;
    union {
        struct
        {
            StrafeDir strafe;
        } crouch;
        struct
        {
            StrafeDir strafe;
        } walk;
        struct
        {
            bool airJump;
        } jump;
        struct
        {
            vec3s pos, ledge_pos;
            float dist;
            u8    closeEnough, doRoll;
        } mantle;
        struct
        {
            float boost;
        } slide;
        struct
        {
            vec3s     wallNormal;
            WallTouch wallTouch;
        } wallrun;
    } as;
    vec3s enterVel, dir;
} MoveStateData;
typedef struct SolMove3
{
    MovementKind kind;
    MoveState    state;
    vec3s        updir, lastTouch, knockVel, lastMoveDir;

    float baseHeight, targetHeight;
    float speedMod, frictionMod, gravityMod, knockDur;

    float wallDot, groundDot;
    float airtime, groundtime;

    bool          wantsJump, jumpPressedLastFrame;
    MoveStateData stateData[MOVE_STATE_COUNT];
} SolMove3;

typedef struct SolModel
{
    ModelKind kind;
    vec4s     color;
    bool      is2d;
    float     xOffset, yOffset, yawOffset;
    u32       leftWeaponEnt, rightWeaponEnt;
} SolModel;

typedef struct SolAnim
{
    SolPose   pose;
    SolPoseE  lastPose;
    AnimLayer layers[ANIM_LAYER_COUNT];
    bool      hasLastPose;
} SolAnim;

typedef struct SolPlayer
{
    int localIdx;
} SolPlayer;

typedef struct SolRemote
{
    int remoteId;
} SolRemote;

typedef struct
{
    float lastEntered, elapsed, duration, accum;
    float attacktimer;
} AiStateData;
typedef struct SolAi
{
    vec3s       dirToTarget;
    AiState     state;
    u32         target, justHitUs;
    float       distToTarget, dropAggroTimer, lastHit;
    AiStateData stateData[AISTATE_COUNT];
} SolAi;

typedef struct SolEvent
{
    EventKind kind;
    u32       entA, entB;
    union {
        struct
        {
            vec3s pos, normal, vel;
            u32   entA, entB;
        } collision;
        struct
        {
            float damage;
            u32   entA, entB;
        } death;
        struct
        {
            vec3s pos;
            vec4s color;
            u32   kind, entA, entB;
            float scale, duration;
        } fx;
        struct
        {
            u32   kind;
            vec3s pos;
            float volume;
        } sound;
        struct
        {
            u32   ent;
            vec3s pos;
        } respawn;
        struct
        {
            u32 entId;
            u32 slot;
            u32 ability;
            u32 rarity;
        } equip;
        struct
        {
            u32   entA, entB;
            float damageDealt;
        } score;
        struct
        {
            u32 kind;
            u32 interactor, interactee;
        } interact;
    } as;
} SolEvent;

typedef struct
{
    union {
        struct
        {
            vec3s     enterDir;
            StrafeDir strafe;
        } dash;
        struct
        {
            vec3s laserPoints[16];
            int   laserPointCount;
            vec3s laserPointsVisual[16];
            int   laserPointCountVisual;
        } laser;
        struct
        {
            vec3s whipPoints[16];
            int   whipPointCount;
        } whip;
    } as;

    AbilityState kind;
    float        elapsed, accum, power, recover;
    float        duration;
    double       lastEntered, lastExited;
    u32          stage;
    u32          hitSessionGen;
    bool         held, doesHit;
} AbilityStateData;
typedef struct SolAbility
{
    int              state, activeSlot;
    int              action_map[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} SolAbility;

typedef struct
{
    u8    kind, inf, harmful;
    u32   source;
    float ttl, duration, accum;
    float freq, power;
} Buff;
typedef struct SolBuff
{
    Buff buffs[MAX_BUFFS];
    u32  count;
    u32  activeKindsMask;
} SolBuff;

typedef struct SolTimer
{
    float elapsed, duration;
} SolTimer;

typedef struct SolAudio
{
    int count, handle;
} SolAudio;

typedef struct SolParent
{
    u32     parentId, active;
    vec3s   localOffset;
    versors localQuat;
    char    boneFollow[16];
} SolParent;

typedef struct SolOwner
{
    u32 ownerId;
    u32 team;
} SolOwner;

typedef struct SolCombat
{
    vec3s  respawnPos;
    float  maxHealth, maxEnergy, maxMana;
    float  health, energy, mana;
    float  energyRegen;
    bool   doesRespawn;
    float  respawnTime;
    double deathTime, lastHitTime;

    int leftWeaponEnt, rightWeaponEnt;

    u16   flags;
    u32   hitPauseDiminish;
    bool  hitEnts[128];
    float hitPause, baseAnimRate;
} SolCombat;

typedef struct SolReplication
{
    u8  auth;
    u32 prefabKind;
} SolReplication;

typedef struct
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        ttl, scale, span, speed, delay;
    float        rot, rotspeed, offset, scalein, scaleout, fadein, fadeout;
    u32          randScale, followId, randLife, randScaleout;
} Particle;

typedef struct
{
    EmitterKind emitterKind;
    vec3s       pos, vel;
    float       ttl, rate, accumulator;
    Particle    particle;
    u32         burst, inf, followId, rateBurst;
    u32         followIdGen;
} Emitter;

typedef struct SolEmitter
{
    Emitter emitters[MAX_EMITTERS];
    u32     emitterCount;
} SolEmitter;

typedef struct
{
    View2dKind kind;
    vec4s      dims, offset;
    vec4s      color;
    vec4s      hoverColor, clickColor, toggleColor;
    float      fill, scale, textWidth, border;
    float      hoverAnim, clickAnim;
    float      targetFill, fillSpeed;
    u32        zindex;
    u8         textureID, flags;
    vec2s      textureUV;
    char       text[64];
} View2;
typedef struct SolView2
{
    View2 views[MAX_VIEWS];
    u8    count;
    u8    zindex;
} SolView2;

typedef struct SolTracker
{
    World     *world;
    u32        entId;
    GetterFunc getters[MAX_TRACKER_GETTERS];
} SolTracker;

typedef struct SolProjectile
{
    ProjectileKind kind;
    u32            bounces;
    float          power;
    float          explodeRadius;
    u32            hitFX, explodeHitFX;

    SolCallback callback;
    u32         callbackFlags;
    SolHit      directHit;
    SolHit      explosionHit;
} SolProjectile;

typedef struct SolHudItem
{
    int idx;
} SolHudItem;

typedef struct SolHudSlot
{
    int  slot;
    bool onCooldown;
} SolHudSlot;

typedef struct SolTooltip
{
    TooltipKind kind;
} SolTooltip;

typedef struct SolZone
{
    u32   kind;
    float duration, rate, value, radius;
    float accum;
} SolZone;

typedef struct SolSlider
{
    float min_val;
    float max_val;
    float current_val;
} SolSlider;

typedef struct SolBuilder
{
    bool    placing, doesSnap;
    float   scale;
    vec3s   placePos;
    versors placeRot;
    u32     model;
} SolBuilder;

// ==========================================
// 2. X-MACRO COMPONENT LIST
// X(Type, EnumFlag)
// ==========================================

#define SOL_COMPONENT_LIST(X)                                                                                          \
    X(SolActive, HAS_SolActive)                                                                                        \
    X(SolXform, HAS_SolXform)                                                                                          \
    X(SolController, HAS_SolController)                                                                                \
    X(SolBody2, HAS_SolBody2)                                                                                          \
    X(SolBody3, HAS_SolBody3)                                                                                          \
    X(SolModel, HAS_SolModel)                                                                                          \
    X(SolAnim, HAS_SolAnim)                                                                                            \
    X(SolCamera, HAS_SolCamera)                                                                                        \
    X(SolInteract, HAS_SolInteract)                                                                                    \
    X(SolMove3, HAS_SolMove3)                                                                                          \
    X(SolPlayer, HAS_SolPlayer)                                                                                        \
    X(SolRemote, HAS_SolRemote)                                                                                        \
    X(SolAi, HAS_SolAi)                                                                                                \
    X(SolAbility, HAS_SolAbility)                                                                                      \
    X(SolBuff, HAS_SolBuff)                                                                                            \
    X(SolTimer, HAS_SolTimer)                                                                                          \
    X(SolEvent, HAS_SolEvent)                                                                                          \
    X(SolAudio, HAS_SolAudio)                                                                                          \
    X(SolParent, HAS_SolParent)                                                                                        \
    X(SolOwner, HAS_SolOwner)                                                                                          \
    X(SolCombat, HAS_SolCombat)                                                                                        \
    X(SolReplication, HAS_SolReplication)                                                                              \
    X(SolEmitter, HAS_SolEmitter)                                                                                      \
    X(SolSlider, HAS_SolSlider)                                                                                        \
    X(SolView2, HAS_SolView2)                                                                                          \
    X(SolTracker, HAS_SolTracker)                                                                                      \
    X(SolProjectile, HAS_SolProjectile)                                                                                \
    X(SolHudSlot, HAS_SolHudSlot)                                                                                      \
    X(SolHudItem, HAS_SolHudItem)                                                                                      \
    X(SolTooltip, HAS_SolTooltip)                                                                                      \
    X(SolZone, HAS_SolZone)                                                                                            \
    X(SolBuilder, HAS_SolBuilder)

typedef enum
{
#define AS_ENUM(type, flag) flag,
    SOL_COMPONENT_LIST(AS_ENUM)
#undef AS_ENUM
    COMPONENT_COUNT
} WorldComponents;

SolXform *Sol_Xform_Add(World *world, int id, vec3s pos);
SolAnim  *Sol_Anim_Add(World *world, int id);
SolBody3 *Sol_Body3_Add(World *world, int id);