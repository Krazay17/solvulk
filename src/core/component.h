/*
 * File: component.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-11
 * 
*/
#pragma once

#include "sol/types.h"

#define MAX_SYSTEMS 64
#define MAX_BUFFS 64
#define MAX_VIEWS 10
#define MAX_TRACKER_GETTERS 2
#define MAX_EMITTERS 8
#define MAX_INTERACTS 32

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
    vec3s wishdir, wishdir2, aimdir, aimpos, lookdir;
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
typedef struct ScAi
{
    u8 kind;
    AiState state;
    vec3s dirToTarget;
    u32 target, last_target, justHitUs;
    float aggroRange;
    float distToTarget, dropAggroTimer, lastHit;
    AiStateData stateData[AISTATE_COUNT];
} ScAi;

typedef struct ScBody3
{
    Shape3 shape;
    bool ignoreFriendly;
    u32 ignoreEnt;
    vec3s vel, impulse, force, dims, gravity;
    float mass, invMass, restitution;
    u32 mask, base_mask;
    u32 ray_mask, ray_base_mask;
} ScBody3;

typedef struct ScBody2
{
    Shape2 shape;
    vec3s vel, dims, gravity, force, impulse;
    u32 mask;
    float restitution;
    bool ignoreWindow;
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

typedef struct ScEvent
{
    EventKind kind;
    u32 entA, entB;
    union {
        struct
        {
            vec3s pos, normal, vel;
            u32 entA, entB;
        } collision;
        struct
        {
            float damage;
            u32 entA, entB;
        } death;
        struct
        {
            vec3s pos;
            vec4s color;
            u32 kind, entA, entB;
            float scale, duration;
        } fx;
        struct
        {
            u32 kind;
            vec3s pos;
            float volume;
        } sound;
        struct
        {
            u32 ent;
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
            u32 entA, entB;
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

    float accum, power;

    float elapsed, duration;
    float recover, recoverDuration;
    float cooldown, cooldownRemaining;

    u8 stage;
    bool held;
} AbilityStateData;
typedef struct ScAbility
{
    int state, activeSlot, slots;
    int action_map[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} ScAbility;

typedef struct
{
    u8 kind, inf, harmful;
    u32 source;
    float ttl, duration, accum;
    float freq, power;
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

    u32 hitPauseDiminish;
    float hitPause;

    int hitSession;
} ScCombat;

typedef struct ScReplication
{
    u8 auth;
    u32 prefabKind;
} ScReplication;

typedef struct
{
    ParticleKind kind;
    vec3s pos, vel;
    vec4s color;
    float ttl, scale, span, speed, delay;
    float rot, rotspeed, offset, scalein, scaleout, fadein, fadeout;
    u32 randScale, followId, randLife, randScaleout;
} Particle;

typedef struct
{
    EmitterKind emitterKind;
    vec3s pos, vel;
    float ttl, rate, accumulator;
    Particle particle;
    u32 burst, inf, followId, rateBurst;
    u32 followIdGen;
} Emitter;

typedef struct ScEmitter
{
    Emitter emitters[MAX_EMITTERS];
    u32 emitterCount;
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
    vec4s hoverColor, downColor, clickColor, toggleColor;
    float hoverAnim, downAnim, clickAnim;
    float fill, scale, textWidth, border;
    float targetFill, fillSpeed;
    u8 textureID, flags;
    vec2s textureUV;
    char text[64];
} View2;
typedef struct ScView2
{
    View2 views[MAX_VIEWS];
    u8 count;
    u32 layer;
} ScView2;

typedef struct ScView3
{
    View3Kind kind;
    vec3s dims, offset;
    vec4s color;
} ScView3;

typedef struct ScTracker
{
    World *world;
    u32 entId;
    GetterFunc getters[MAX_TRACKER_GETTERS];
} ScTracker;

typedef struct ScProjectile
{
    ProjectileKind kind;
    u32 bounces;
    float power;
    float explodeRadius;
    u32 hitFX, explodeHitFX;

    SolCallback callback;
    u32 callbackFlags;
    SolHit directHit;
    SolHit explosionHit;
} ScProjectile;

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
    void *data;
} ScHook;

typedef struct ScRef
{
    u32 kind;
    int index;
} ScRef;

extern const char *ability_state_name[ABILITY_STATE_COUNT];
extern const char *move_state_name[MOVE_STATE_COUNT];
extern const u32 ability_texture_map[ABILITY_STATE_COUNT];
extern const AbilityConfig ability_base[ABILITY_STATE_COUNT];