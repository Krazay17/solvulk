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
    bool isStrafing, isWalking;
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

typedef struct AiBrain
{
    u32 target, justHitUs;
    vec3s target_pos;
    vec3s target_dir;
    float target_dist;
    float target_prev_dist;
    float dropAggroTimer;
} AiBrain;
typedef struct AiLearning
{
    float actionTimer;
    float reward_move;
    float reward_combat;
    AiKnowStateM prev_knows_move;
    AiKnowStateC prev_knows_combat;
    AiActions action_move;
    AiActionsC action_combat;
} AiLearning;
typedef struct ScAi
{
    u8 kind;
    AiState state;
    AiStateData stateData[AISTATE_COUNT];

    float maxHomeRange;
    float aggroRange;
    AiBrain brain;
    AiLearning learning;
} ScAi;

typedef struct ScBody3
{
    Shape3 shape;
    bool ignoreFriendly, is_sensor;
    u32 ignoreEnt;
    vec3s vel, impulse, force, dims, gravity;
    float mass, invMass, restitution;
    u32 mask, base_mask; // group << 16 | mask << 0
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
            // vec3s laserPoints[16];
            // int laserPointCount;
            // vec3s laserPointsVisual[16];
            int laserPointCountVisual;
        } laser;
        struct
        {
#define MAX_BOLT_ANCHORS 8
            vec3s anchor[MAX_BOLT_ANCHORS];
            int current_anchor;
            int bolt;
            int bolt_state;
        } bolt;
    } as;

    AbilityConfig conf;

    float elapsed, accum, power, cooldownRemaining;
    float drawElapsed, drawAccum;
    float hitPause;
    float hitaccum;

    u32 hitgen;
    u32 hitgen2;
    u8 hitPauseDr;
    u8 stage;
    bool held;
} AbilityStateData;
typedef struct ScAbility
{
    int prio_slot;
    int left_weapon;
    int right_weapon;
    u32 state[ABILITY_SLOTS];
    u32 base_actions[ABILITY_SLOTS];
    u32 slotted_actions[ABILITY_SLOTS];
    SolItem slotted_items[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} ScAbility;

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
    u32 priority_ability_slot;
    float priority_ability_duration;
    float target_pitch;
    float pitch;
} ScAnim;

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
    bool shrinkout;
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
    char boneFollow[32];
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
    float respawnTime;

    u32 lastHitBy;

    bool is_dead;
    bool random_spawn;
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

typedef enum
{
    UIKIND_BUTTON,
    UIKIND_SLIDER,
    UIKIND_COUNT,
} UiKind;
typedef struct ScUi
{
    UiKind kind;

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

typedef struct
{
    View3Kind kind;
    vec3s offset;
    vec4s color;
    float scale;
    float duration;
    float _elapsed;
} View3;
typedef struct ScView3
{
    View3 *views_b; // solbuffer
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
    AiKnowStateM prev_knows_move;
    AiKnowStateC prev_knows_combat;
    u32 action_move;
    u32 action_combat;
    float reward_move;
    float reward_combat;
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
    RefKind kind;
    u32 ent_world, ent_id;
    int index;
} ScRef;

#define ABILITY_GROUPS 3

typedef struct ScAbilitybar
{
    float spacing;   // gap between slots within a group
    float group_gap; // gap between group frames (was group_spacing, now excludes padding)
    vec2s slot_dims;
    vec2s frame_pad; // padding inside each group frame
    int slots_per_group[ABILITY_GROUPS];
    int slotted_ents[ABILITY_SLOTS]; // keep if you already have it
} ScAbilitybar;

typedef enum
{
    WEAPONKIND_CLAW,
    WEAPONKIND_SCYTHE,
    WEAPONKIND_COUNT,
} WeaponKind;
typedef struct
{
    WeaponKind kind;
    bool update_trail;
} ScWeapon;
typedef int (*MakeWeapon)(World *, int owner, int slot);
extern const MakeWeapon Make_Weapon[];

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

typedef struct HitgenRow HitgenRow;
typedef struct SlHitgen
{
    u32 global;
    HitgenRow *rows[MAX_ENTS];
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

typedef enum
{
    RIBBONKIND_BASIC,
    RIBBONKIND_FIRE,
    RIBBONKIND_LIGHTNING,
    RIBBONKIND_WEAPON_TRAIL,
    RIBBONKIND_LIGHTNING_WEAPON_TRAIL,
    RIBBONKIND_WEAPON_TRAIL_RED,
    RIBBONKIND_WEAPON_TRAIL_COLORRING,
    RIBBONKIND_COUNT,
} RibbonKind;
typedef enum
{
    RIBBONFLAG_NOFACECAM = 1,
} RibbonFlag;
#define MAX_RIBBON_SEGMENTS 64
typedef struct Ribbon
{
    RibbonKind kind;
    vec3s points[MAX_RIBBON_SEGMENTS];
    float thickness;
    vec4s color;
    u32 texture;
    float lifespan;
    u32 entA, entB;
    RibbonFlag flags;
    float spin[MAX_RIBBON_SEGMENTS];
    u32 sheets;
    float pan;
    float alpha_curve;
    float scale_curve;

    int _point_count;
    float _point_elapsed[MAX_RIBBON_SEGMENTS];
    float _elapsed;
    float _update_timer;
} Ribbon;

typedef struct
{
    Ribbon *ribbons;
} SlRibbon;

typedef struct
{
    Ribbon ribbon;
    float rate;
    float _accum;
} ScRibbon;

typedef enum
{
    CHAINHITKIND_LIGHTNING,
    CHAINHITKIND_COUNT,
} ChainhitKind;
typedef struct Chainhit Chainhit;
typedef struct
{
    Chainhit *chainhits;
} SlChainhit;

#define SOL_SINGLETON_LIST(X)                                                                                          \
    X(SlEvent, SlEvent_Init, SlEvent_Deinit)                                                                           \
    X(SlDebug, SlDebug_Init, SlDebug_Deinit)                                                                           \
    X(SlHitgen, SlHitgen_Init, SlHitgen_Deinit)                                                                        \
    X(SlEmitter, SlEmitter_Init, SlEmitter_Deinit)                                                                     \
    X(SlContacts2, SlContacts2_Init, SlContacts2_Deinit)                                                               \
    X(SlRibbon, SlRibbon_Init, SlRibbon_Deinit)                                                                        \
    X(SlChainhit, SlChainhit_Init, SlChainhit_Deinit)                                                                  \
    X(SlSpatial, SlSpatial_Init, SlSpatial_Deinit)

#define SOL_COMPONENT_LIST(X)                                                                                          \
    X(SlEvent, HAS_SlEvent)                                                                                            \
    X(SlDebug, HAS_SlDebug)                                                                                            \
    X(SlSpatial, HAS_SlSpatial)                                                                                        \
    X(SlHitgen, HAS_SlHitgen)                                                                                          \
    X(SlEmitter, HAS_SlEmitter)                                                                                        \
    X(SlContacts2, HAS_SlContacts2)                                                                                    \
    X(SlRibbon, HAS_SlRibbon)                                                                                          \
    X(SlChainhit, HAS_SlChainhit)                                                                                      \
                                                                                                                       \
    X(ScActive, HAS_ScActive)                                                                                          \
    X(ScHook, HAS_ScHook)                                                                                              \
    X(ScCmd, HAS_ScCmd)                                                                                                \
    X(ScUi, HAS_ScUi)                                                                                                  \
    X(ScMeta, HAS_ScMeta)                                                                                              \
    X(ScTeam, HAS_ScTeam)                                                                                              \
    X(ScPlayer, HAS_ScPlayer)                                                                                          \
    X(ScRemote, HAS_ScRemote)                                                                                          \
    X(ScAi, HAS_ScAi)                                                                                                  \
    X(ScAilearn, HAS_ScAilearn)                                                                                        \
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
    X(ScAudio, HAS_ScAudio)                                                                                            \
    X(ScParent, HAS_ScParent)                                                                                          \
    X(ScOwner, HAS_ScOwner)                                                                                            \
    X(ScCombat, HAS_ScCombat)                                                                                          \
    X(ScReplication, HAS_ScReplication)                                                                                \
    X(ScEmitter, HAS_ScEmitter)                                                                                        \
    X(ScSlider, HAS_ScSlider)                                                                                          \
    X(ScView2, HAS_ScView2)                                                                                            \
    X(ScView3, HAS_ScView3)                                                                                            \
    X(ScProjectile, HAS_ScProjectile)                                                                                  \
    X(ScHudslot, HAS_ScHudslot)                                                                                        \
    X(ScHuditem, HAS_ScHuditem)                                                                                        \
    X(ScTooltip, HAS_ScTooltip)                                                                                        \
    X(ScZone, HAS_ScZone)                                                                                              \
    X(ScRef, HAS_ScRef)                                                                                                \
    X(ScAbilitybar, HAS_ScAbilitybar)                                                                                  \
    X(ScRibbon, HAS_ScRibbon)                                                                                          \
    X(ScWeapon, HAS_ScWeapon)                                                                                          \
    X(ScBuilder, HAS_ScBuilder)

typedef enum
{
#define AS_ENUM(type, flag) flag,
    SOL_COMPONENT_LIST(AS_ENUM)
#undef AS_ENUM
    COMPONENT_COUNT
} SolComponents;

#define SINGLETON_FWD(Type, InitFn, DeinitFn)                                                                          \
    void InitFn(World *world, Type *self);                                                                             \
    void DeinitFn(Type *self);
SOL_SINGLETON_LIST(SINGLETON_FWD)
#undef SINGLETON_FWD

void World_InitSingletons(World *world);
void World_DeinitSingletons(World *world);

typedef struct BaseSparseSet
{
    int cnt;
    int cap;
    int *sparse;
    int *dense;
    void *data;
    size_t data_size;
} BaseSparseSet;
#define SPARSE_SET_STRUCT(T)                                                                                           \
    typedef struct SparseSet_##T                                                                                       \
    {                                                                                                                  \
        int cnt;                                                                                                       \
        int cap;                                                                                                       \
        int *sparse;                                                                                                   \
        int *dense;                                                                                                    \
        T *data;                                                                                                       \
        size_t data_size;                                                                                              \
    } SparseSet_##T

// Declare all SparseSet structs
#define DECLARE_SPARSE_STRUCTS(type, flag) SPARSE_SET_STRUCT(type);
SOL_COMPONENT_LIST(DECLARE_SPARSE_STRUCTS)
#undef DECLARE_SPARSE_STRUCTS

void Sol_Comp_InitAll(World *world);
void Sol_Comp_FreeAll(World *world);

bool Sol_Comp_HasE(World *world, int id, u64 idx);
void *Sol_Comp_GetE(World *world, int id, u64 idx);
void *Sol_Comp_AddE(World *world, int id, u64 idx);
void Sol_Comp_RemE(World *world, int id, u64 idx);

#define Sol_Comp_Has(w, id, type) ((u32)(id) < (u32)((w)->maxEntities) && (((w)->masks[id] & 1ULL << HAS_##type)) != 0)
#define Sol_Comp_Get(w, id, type) ((type *)Sol_Comp_GetE((w), (id), HAS_##type))
#define Sol_Comp_Add(w, id, type) ((type *)Sol_Comp_AddE((w), (id), HAS_##type))
#define Sol_Comp_Rem(w, id, type) (Sol_Comp_RemE((w), (id), HAS_##type))
#define Sol_Comp_Set(w, type) ((SparseSet_##type *)((w)->components[HAS_##type]))

// use c->
#define forc(w, id, type) for (type *c = Sol_Comp_Get(w, id, type); c; c = NULL)
