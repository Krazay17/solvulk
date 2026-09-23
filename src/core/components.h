/*
 * File: components.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-11
 *
 */
#pragma once

#include "sol/types.h"

#define MAX_BUFFS 32
#define MAX_VIEWS 10
#define MAX_EMITTERS 8

typedef struct ScActive
{
    int active_at_tick;
    double time_activated;
} ScActive;

typedef enum
{
    CMDKIND_PLAYER_LOCAL,
    CMDKIND_PLAYER_REMOTE,
    CMDKIND_NPC,
} CmdKind;

typedef struct ScCmd
{
    u8 kind;
    bool isStrafing;
    SolActions actionState;
    SolActions action_state_prev;
    u32 reaction_state;
    int target, interact;
    float yaw, pitch;
    vec3s wishdir, wishdir2, aimdir, aimpos, lookdir, leftdir;
} ScCmd;

typedef struct ScMeta
{
    char name[64];
} ScMeta;

typedef struct ScTeam
{
    u32 faction;
    u32 team;
    u32 party[4];
    u32 party_count;
} ScTeam;

typedef struct ScPlayer
{
    int level;
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

// #define AIKNOWS_LIST(X)                                                                                                \
//     X(STEPFRONT)                                                                                                       \
//     X(LEDGEFRONT)                                                                                                      \
//     X(WALLFRONT)                                                                                                       \
//     X(WALLLEFT)                                                                                                        \
//     X(WALLRIGHT)                                                                                                       \
//     X(WALLBACK)                                                                                                        \
//     X(TARGETFRONT)                                                                                                     \
//     X(TARGETLEFT)                                                                                                      \
//     X(TARGETRIGHT)                                                                                                     \
//     X(TARGETBACK)                                                                                                      \
//     X(TARGETABOVE)                                                                                                     \
//     X(TARGETBELOW)                                                                                                     \
//     X(TARGETCLOSE)                                                                                                     \
//     X(TARGETMID)                                                                                                       \
//     X(TARGETFAR)

// typedef enum
// {
// #define X(name) AIKNOWS_BIT_##name,
//     AIKNOWS_LIST(X)
// #undef X
//     AIKNOWS_COUNT,
// } AiKnowsBit;

// typedef enum
// {
// #define X(name) AIKNOWS_##name = (1 << AIKNOWS_BIT_##name),
//     AIKNOWS_LIST(X)
// #undef X
// AIKNOWS_STATE_COUNT,
// } AiKnows;

typedef struct AiBrain
{
    u32 target, justHitUs;
    vec3s target_pos;
    vec3s target_dir;
    float target_dist;
    float target_prev_dist;
    float dropAggroTimer;
} AiBrain;
typedef struct ScAi
{
    u8 kind;
    AiState state;
    AiStateData stateData[AISTATE_COUNT];
    AiBrain brain;
    float aggroRange;

    float actionTimer;
    float reward;
    AiKnowState knows;
    AiActions aiaction;
} ScAi;

typedef struct ScBody3
{
    Shape3 shape;
    bool ignoreFriendly, is_sensor;
    u32 ignoreEnt;
    vec3s vel, impulse, force, dims, gravity;
    float mass, invMass, restitution;
    u32 mask, base_mask;
    u32 ray_mask, ray_base_mask;
    u32 flag_destroy;
} ScBody3;

typedef struct ScBody2
{
    Shape2 shape;
    vec3s vel, dims, gravity, force, impulse;
    u32 mask;
    float mass, invMass, restitution;
    bool ignoreWindow, isStatic, isSensor;
    int zindex;
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
    uint16_t state;
    uint16_t state_prev;
    bool is_local;

    int interactor;
    float range, duration;
    vec3s drag_offset, down_pos;

    double down_start_time;
    double down_end_time;
    double hover_start_time;
    double hover_end_time;
} ScInteract;

typedef struct
{
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
            u8 closeEnough, doRoll;
        } mantle;
        struct
        {
            float boost;
        } slide;
        struct
        {
            vec3s wallNormal;
            WallTouch wallTouch;
        } wallrun;
        struct
        {
            vec3s velocity;
        } fall;
    } as;
    float coyote;
    double lastEntered, lastExited;
    float elapsed, accum;
    vec3s vel;
} MoveStateData;
typedef struct ScMove3
{
    MovementKind kind;
    MoveState state;
    vec3s updir, lastTouch, knockVel, lastMoveDir, groundNorm, vel;

    float baseHeight, targetHeight;
    float speedMod, frictionMod, gravityMod, knockDur;

    float wallDot;
    float airtime, groundtime;
    float groundDist;

    bool wantsJump, jumpPressedLastFrame;
    MoveStateData stateData[MOVE_STATE_COUNT];
} ScMove3;

typedef struct ScMove2
{
    MovementKind kind;
    MoveState state;

    float baseHeight, targetHeight;
    float speedMod, frictionMod, gravityMod, knockDur;

    float wallDot;
    float airtime, groundtime;

    bool wantsJump, jumpPressedLastFrame;
} ScMove2;

typedef struct ScModel
{
    ModelKind kind;
    vec4s color;
    bool is2d;
    float xOffset, yOffset, yawOffset;
} ScModel;

typedef struct ScAnim
{
    SolPose pose;
    SolPoseE lastPose;
    AnimLayer layers[ANIM_LAYER_COUNT];
    bool hasLastPose;
} ScAnim;

typedef struct
{
    union {
        struct
        {
            float explode_damage;
        } fireball;
        struct
        {
            vec3s dir;
            StrafeDir strafe;
        } dash;
        struct
        {
            vec3s laserPoints[16];
            int laserPointCount;
            vec3s laserPointsVisual[16];
            int laserPointCountVisual;
        } laser;
        struct
        {
            vec3s whipPoints[16];
            int whipPointCount;
        } whip;
    } as;

    float elapsed, accum, power, recoverRemaining, cooldownRemaining;

    u32 hitgen;
    u8 stage;
    bool held;
    AbilityConfig conf;
} AbilityStateData;
typedef struct ScAbility
{
    int state, activeSlot, slots;
    int base_actions[ABILITY_SLOTS];
    int slotted_actions[ABILITY_SLOTS];
    SolItem slotted_items[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} ScAbility;

typedef struct
{
    u8 kind, inf;
    bool hasUpdated;
    u32 source;
    float duration, rate, power, damage;
    float elapsed, accum;
} Buff;
typedef struct ScBuff
{
    Buff buffs[MAX_BUFFS];
    u32 count;
    u32 activeKindsMask;
} ScBuff;

typedef struct ScTimer
{
    float elapsed, duration;
    bool destroy;
} ScTimer;

typedef struct ScAudio
{
    int count, handle;
} ScAudio;

typedef struct ScParent
{
    u32 parentId, active;
    vec3s localOffset;
    versors localQuat;
    char boneFollow[16];
} ScParent;

typedef struct ScOwner
{
    u32 ownerId;
    u32 ownerGen;
} ScOwner;

typedef struct ScCombat
{
    u32 kind;
    float health, healthMax, healthRegen;
    float energy, energyMax, energyRegen;
    float mana, manaMax, manaRegen;

    float damageTaken;
    float healingTaken;

    float damageDone;
    float healingDone;

    double deathTime, lastHitTime;
    vec3s respawnPos;
    float respawnTime;

    u32 lastHitBy;
    u32 hitPauseDiminish;
    float hitPause;

    bool is_dead;
} ScCombat;

typedef struct ScReplication
{
    u8 auth;
    u32 prefabKind;
} ScReplication;

typedef struct ScEmitter
{
    Emitter emitters[MAX_EMITTERS];
} ScEmitter;

enum UiKind
{
    UIKIND_BUTTON,
    UIKIND_SLIDER,
};
typedef struct ScUi
{
    u8 kind;
    float value;
} ScUi;

typedef struct
{
    View2Kind kind;
    vec4s dims, offset;
    vec4s color;
    vec4s hoverColor, downColor, activeColor;
    float hoverAnim, downAnim, activeAnim;
    float fill, scale, textWidth, border;
    float targetFill, fillSpeed;
    float desat;
    u8 textureID, flags, layer;
    vec4s textureUV;
    char text[64];
} View2;
typedef struct ScView2
{
    View2 views[MAX_VIEWS];
    u8 count;
} ScView2;

typedef struct ScView3
{
    View3Kind kind;
    vec3s offset;
    vec4s color;
    float scale;
} ScView3;

typedef struct ScProjectile
{
    ProjectileKind kind;
    u32 mask;
    u32 hitgen;
    u32 bounces;
    SolHit hit;
    SolHit aoe_hit;
    float radius;
    float power;
    Hook hook;
} ScProjectile;

typedef struct ScAilearn
{
    AiKnowState knows;
    u32 action;
    float reward;
} ScAilearn;

typedef struct ScHuditem
{
    int idx;
} ScHuditem;

typedef struct ScHudslot
{
    int slot;
    bool onCooldown;
} ScHudslot;

typedef struct ScTooltip
{
    TooltipKind kind;
} ScTooltip;

typedef struct ScZone
{
    u32 kind;
    float duration, rate, value, radius;
    float accum;
    float expand_rate;

    SolHit hit;
    u32 hitgen;
} ScZone;

typedef struct ScSlider
{
    vec3s offset, axis;
    float track_len;
    float step;
    float value;
} ScSlider;

typedef struct ScBuilder
{
    bool placing, doesSnap;
    float scale;
    vec3s placePos;
    versors placeRot;
    u32 model;
} ScBuilder;

typedef struct ScStage
{
    bool isDirty;
} ScStage;

typedef struct ScHook
{
    Hook held;
    Hook pressed;
    Hook update;
    Hook release;
    Hook in_range;
} ScHook;

typedef struct ScRef
{
    u32 kind;
    u32 ent_world, ent_id;
    int index;
} ScRef;

typedef struct ScAbilitybar
{
    int slots;
    vec2s slot_dims;
} ScAbilitybar;

// #################
// #### SINGLES ####
// #################

typedef struct SlEvent
{
    SolEvent *events;
} SlEvent;

typedef struct DebugLine
{
    SolLine line;
    float ttl;
} DebugLine;

typedef struct DebugSphere
{
    SolSphere sphere;
    float ttl;
} DebugSphere;
typedef struct SlDebug
{
    DebugLine *lines;
    DebugSphere *spheres;
} SlDebug;

typedef struct SpatialGrid SpatialGrid;
typedef struct SlSpatial
{
    SpatialGrid *grid_dynamic;
    SpatialGrid *grid_static;
    SolContact *contacts;
    ThreadContactBuffer *threadContacts;
    IdBuffer *threadIds;
    SolTri *tris_static;
    u32 *build_ids;
    vec3s *build_mins;
    vec3s *build_maxs;
} SlSpatial;

typedef struct
{
    u32 gen;          // this attacker's current session generation
    u32 *hit_targets; // solb_ buffer: which target ids have been marked this session
} HitgenRow;

typedef struct SlHitgen
{
    u32 global;
    HitgenRow rows[MAX_ENTS]; // one row per potential attacker, allocated lazily
} SlHitgen;

typedef struct SlEmitter
{
    Emitter *emitters;
    Particle *particles;
} SlEmitter;

typedef struct SlContacts2
{
    SolContact *contacts;
} SlContacts2;
