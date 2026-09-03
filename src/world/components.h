#pragma once

#include "sol/types.h"

#define MAX_SYSTEMS 64
#define MAX_BUFFS 64
#define MAX_VIEWS 10
#define MAX_TRACKER_GETTERS 2
#define MAX_EMITTERS 8

// ==========================================
// 1. COMPONENT DATA STRUCTS
// ==========================================
typedef struct ScActive
{
    int    active_at_tick;
    double time_activated;
} ScActive;

typedef struct ScXform
{
    vec3s   last_pos, pos, draw_pos;
    vec3s   last_sca, sca, draw_sca;
    versors last_rot, rot, draw_rot;
} ScXform;

typedef struct ScCmd
{
    SolActions actionState;
    bool       isStrafing;
    int        target;
    float      yaw, pitch;
    vec3s      wishdir, wishdir2, aimdir, aimpos, lookdir;
} ScCmd;

typedef struct ScPlayer
{
    float yaw, pitch;
} ScPlayer;

typedef struct ScRemote
{
    int remoteId;
} ScRemote;

typedef struct
{
    float lastEntered, elapsed, duration, accum;
    float attacktimer;
} AiStateData;
typedef struct ScAi
{
    vec3s       dirToTarget;
    AiState     state;
    u32         target, justHitUs;
    float       distToTarget, dropAggroTimer, lastHit;
    AiStateData stateData[AISTATE_COUNT];
} ScAi;

typedef struct ScBody3
{
    Shape3 shape;
    bool   ignoreFriendly;
    vec3s  vel, impulse, force, dims, gravity;
    float  mass, invMass, restitution;
    u32    mask, base_mask;
    u32    ray_mask, ray_base_mask;
} ScBody3;

typedef struct ScBody2
{
    Shape2 shape;
    vec3s  vel, dims, gravity, force, impulse;
    u32    mask;
    bool   collide_window;
    float  restitution;
} ScBody2;

typedef struct ScCamera
{
    vec3s pos, anchor;
    vec3s dir;
    vec3s up, right;
    float current_distance, desired_distance;
    float current_offset, desired_offset;
    float lerpspeed;
    float fov;
    float roll;
} ScCamera;

typedef struct ScInteract
{
    InteractState state;
    double        hover_start_time;
    double        unhover_start_time;
    double        press_start_time;
    double        pressedAccum;
    float         range;
} ScInteract;

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
typedef struct ScMove3
{
    MovementKind kind;
    MoveState    state;
    vec3s        updir, lastTouch, knockVel, lastMoveDir, groundNorm;

    float baseHeight, targetHeight;
    float speedMod, frictionMod, gravityMod, knockDur;

    float wallDot, groundDot;
    float airtime, groundtime;

    bool          wantsJump, jumpPressedLastFrame;
    MoveStateData stateData[MOVE_STATE_COUNT];
} ScMove3;

typedef struct ScMove2
{
    MovementKind kind;
    MoveState    state;

    float baseHeight, targetHeight;
    float speedMod, frictionMod, gravityMod, knockDur;

    float wallDot, groundDot;
    float airtime, groundtime;

    bool wantsJump, jumpPressedLastFrame;
} ScMove2;

typedef struct ScModel
{
    ModelKind kind;
    vec4s     color;
    bool      is2d;
    float     xOffset, yOffset, yawOffset;
    u32       leftWeaponEnt, rightWeaponEnt;
} ScModel;

typedef struct ScAnim
{
    SolPose   pose;
    SolPoseE  lastPose;
    AnimLayer layers[ANIM_LAYER_COUNT];
    bool      hasLastPose;
} ScAnim;

typedef struct ScEvent
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
} ScEvent;

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
typedef struct ScAbility
{
    int              state, activeSlot;
    int              action_map[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} ScAbility;

typedef struct
{
    u8    kind, inf, harmful;
    u32   source;
    float ttl, duration, accum;
    float freq, power;
} Buff;
typedef struct ScBuff
{
    Buff buffs[MAX_BUFFS];
    u32  count;
    u32  activeKindsMask;
} ScBuff;

typedef struct ScTimer
{
    float elapsed, duration;
} ScTimer;

typedef struct ScAudio
{
    int count, handle;
} ScAudio;

typedef struct ScParent
{
    u32     parentId, active;
    vec3s   localOffset;
    versors localQuat;
    char    boneFollow[16];
} ScParent;

typedef struct ScOwner
{
    u32 ownerId;
    u32 team;
} ScOwner;

typedef struct ScCombat
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
} ScCombat;

typedef struct ScReplication
{
    u8  auth;
    u32 prefabKind;
} ScReplication;

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

typedef struct ScEmitter
{
    Emitter emitters[MAX_EMITTERS];
    u32     emitterCount;
} ScEmitter;

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
typedef struct ScView2
{
    View2 views[MAX_VIEWS];
    u8    count;
    u8    zindex;
} ScView2;

typedef struct ScTracker
{
    World     *world;
    u32        entId;
    GetterFunc getters[MAX_TRACKER_GETTERS];
} ScTracker;

typedef struct ScProjectile
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
} ScProjectile;

typedef struct ScHuditem
{
    int idx;
} ScHuditem;

typedef struct ScHudslot
{
    int  slot;
    bool onCooldown;
} ScHudslot;

typedef struct ScTooltip
{
    TooltipKind kind;
} ScTooltip;

typedef struct ScZone
{
    u32   kind;
    float duration, rate, value, radius;
    float accum;
} ScZone;

typedef struct ScSlider
{
    float min_val;
    float max_val;
    float current_val;
} ScSlider;

typedef struct ScBuilder
{
    bool    placing, doesSnap;
    float   scale;
    vec3s   placePos;
    versors placeRot;
    u32     model;
} ScBuilder;

typedef struct ScStage
{
    bool isDirty;
} ScStage;

typedef void (*Hook)(World *, double, int, void *);
typedef struct ScHook
{
    Hook  held;
    Hook  pressed;
    Hook  update;
    void *data;
} ScHook;

// ==========================================
// 2. X-MACRO COMPONENT LIST
// X(Type, EnumFlag)
// ==========================================

#define SOL_COMPONENT_LIST(X)                                                                                          \
    X(ScActive, HAS_ScActive)                                                                                          \
    X(ScXform, HAS_ScXform)                                                                                            \
    X(ScHook, HAS_ScHook)                                                                                              \
    X(ScCmd, HAS_ScCmd)                                                                                                \
    X(ScPlayer, HAS_ScPlayer)                                                                                          \
    X(ScRemote, HAS_ScRemote)                                                                                          \
    X(ScAi, HAS_ScAi)                                                                                                  \
    X(ScBody2, HAS_ScBody2)                                                                                            \
    X(ScBody3, HAS_ScBody3)                                                                                            \
    X(ScStage, HAS_ScStage)                                                                                            \
    X(ScModel, HAS_ScModel)                                                                                            \
    X(ScAnim, HAS_ScAnim)                                                                                              \
    X(ScCamera, HAS_ScCamera)                                                                                          \
    X(ScInteract, HAS_ScInteract)                                                                                      \
    X(ScMove3, HAS_ScMove3)                                                                                            \
    X(ScMove2, HAS_ScMove2)                                                                                            \
    X(ScAbility, HAS_ScAbility)                                                                                        \
    X(ScBuff, HAS_ScBuff)                                                                                              \
    X(ScTimer, HAS_ScTimer)                                                                                            \
    X(ScEvent, HAS_ScEvent)                                                                                            \
    X(ScAudio, HAS_ScAudio)                                                                                            \
    X(ScParent, HAS_ScParent)                                                                                          \
    X(ScOwner, HAS_ScOwner)                                                                                            \
    X(ScCombat, HAS_ScCombat)                                                                                          \
    X(ScReplication, HAS_ScReplication)                                                                                \
    X(ScEmitter, HAS_ScEmitter)                                                                                        \
    X(ScSlider, HAS_ScSlider)                                                                                          \
    X(ScView2, HAS_ScView2)                                                                                            \
    X(ScTracker, HAS_ScTracker)                                                                                        \
    X(ScProjectile, HAS_ScProjectile)                                                                                  \
    X(ScHudslot, HAS_ScHudslot)                                                                                        \
    X(ScHuditem, HAS_ScHuditem)                                                                                        \
    X(ScTooltip, HAS_ScTooltip)                                                                                        \
    X(ScZone, HAS_ScZone)                                                                                              \
    X(ScBuilder, HAS_ScBuilder)

typedef enum
{
#define AS_ENUM(type, flag) flag,
    SOL_COMPONENT_LIST(AS_ENUM)
#undef AS_ENUM
    COMPONENT_COUNT
} WorldComponents;

ScXform *Sol_Xform_Add(World *world, int id, vec3s pos);
ScAnim  *Sol_Anim_Add(World *world, int id);
ScBody3 *Sol_Body3_Add(World *world, int id);