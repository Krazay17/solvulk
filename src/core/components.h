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
    SolActions actions;
    float yaw;
    float pitch;
    bool strafing;
} ScPlayer;

typedef struct ScUser
{
    int uid;
} ScUser;

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
    bool inactive;
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

// #define MAX_INVENTORY_ITEMS 0xffff
typedef struct ScInventory
{
    //  int item_count;
    SolItem *items;
    int equipped_items[ABILITY_SLOTS];
    u32 next_item_id;

    bool dirty;
    bool dirty_equip;
} ScInventory;

typedef struct ScEquip
{
    int left_weapon;
    int right_weapon;
} ScEquip;

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
        struct
        {
            float accum;
            float rate;
            float amount;
        } heal;
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
    u32 state[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} ScAbility;

typedef struct ScCamera
{
    vec3s pos, anchor;
    vec3s dir;
    vec3s up, right;
    float current_distance, desired_distance;
    float current_offset, desired_offset;
    float y_offset;
    float lerpspeed;
    float fov;
    float roll;
} ScCamera;

typedef struct ScInteract
{
    u8 pending;
    u8 state;
    u8 state_prev;
    u8 state_just;
    InteractFlags flags;
    bool is_local;

    int interactor;
    float range, duration;
    vec3s drag_offset;
    vec3s drag_target;

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
    bool transparent;
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
    int handle;
    char bone_follow[32];
} ScAudio;

typedef struct ScParent
{
    u32 parentId;
    vec3s localOffset;
    versors localQuat;
    vec3s localScale;
    char boneFollow[32];
} ScParent;
extern const ScParent parent_base;

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

    float energyRegenDelay;

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
    vec2s dims, offset;
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

typedef enum
{
    POWERKIND_HEALTH,
    POWERKIND_ENERGY,
} PowerKind;
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
    Hook clicked;
    Hook update;
    Hook in_range;
} ScHook;

typedef struct ScRef
{
    RefKind kind;
    u32 id;
    u32 world;
    u32 flags;
    u32 index;
    u32 uid;
} ScRef;

typedef struct ScAbilitybar
{
    vec2s slot_dims;
    int slot_count;
    int ability_slots[ABILITY_SLOTS];
    int slotted_ents[ABILITY_SLOTS];
} ScAbilitybar;

typedef struct ScUiItem
{
    u8 mode; // 0=inv, 1=loose, 2=equipped, 3=dragging
    u32 world;
    u32 ent;
    u32 uid;
} ScUiItem;

#define MAX_WEAPON_BLADE_BONES 6
typedef struct
{
    WeaponKind kind;
    int equipped_slot;
    int trace_bones_idx[MAX_WEAPON_BLADE_BONES];
    int trace_point_count;
    bool update_trail;
    bool extending;
} ScWeapon;

// #################
// #### SINGLES ####
// #################

typedef void (*EventHandler)(World *world, SolEvent *event);
typedef struct SlEvent
{
    SolEvent *events;
    EventHandler *handlers[EVENTKIND_COUNT];
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

typedef enum
{
    RULESKIND_FFA,
    RULESKIND_TDM,
    RULESKIND_COUNT,
} RulesKind;
typedef enum
{
    RULESPHASE_START,
    RULESPHASE_ACTIVE,
    RULESPHASE_END,
} RulesPhase;
typedef struct
{
    union {
        struct
        {
            int winning_kills;
        } ffa;
    } as;
    RulesKind kind;
    RulesPhase phase;
    int winner;
} SlRules;

typedef struct
{
    u32 kills;
    u32 deaths;
    float damage;
    float healing;
} ScScore;

typedef void (*ComponentInitFn)(World *world, int id, void *c);

#define SOL_SINGLETON_LIST(X)                                                                                          \
    X(SlEvent, SlEvent_Init, SlEvent_Deinit)                                                                           \
    X(SlDebug, SlDebug_Init, SlDebug_Deinit)                                                                           \
    X(SlHitgen, SlHitgen_Init, SlHitgen_Deinit)                                                                        \
    X(SlEmitter, SlEmitter_Init, SlEmitter_Deinit)                                                                     \
    X(SlContacts2, SlContacts2_Init, SlContacts2_Deinit)                                                               \
    X(SlRibbon, SlRibbon_Init, SlRibbon_Deinit)                                                                        \
    X(SlChainhit, SlChainhit_Init, SlChainhit_Deinit)                                                                  \
    X(SlRules, SlRules_Init, SlRules_Deinit)                                                                           \
    X(SlSpatial, SlSpatial_Init, SlSpatial_Deinit)

#define SOL_COMPONENT_LIST(X)                                                                                          \
    X(SlEvent, HAS_SlEvent, Add_None)                                                                                  \
    X(SlDebug, HAS_SlDebug, Add_None)                                                                                  \
    X(SlSpatial, HAS_SlSpatial, Add_None)                                                                              \
    X(SlHitgen, HAS_SlHitgen, Add_None)                                                                                \
    X(SlEmitter, HAS_SlEmitter, Add_None)                                                                              \
    X(SlContacts2, HAS_SlContacts2, Add_None)                                                                          \
    X(SlRibbon, HAS_SlRibbon, Add_None)                                                                                \
    X(SlChainhit, HAS_SlChainhit, Add_None)                                                                            \
    X(SlRules, HAS_SlRules, Add_None)                                                                                  \
                                                                                                                       \
    X(ScActive, HAS_ScActive, Add_None)                                                                                \
    X(ScHook, HAS_ScHook, Add_None)                                                                                    \
    X(ScCmd, HAS_ScCmd, Add_None)                                                                                      \
    X(ScUi, HAS_ScUi, Add_None)                                                                                        \
    X(ScMeta, HAS_ScMeta, Add_None)                                                                                    \
    X(ScTeam, HAS_ScTeam, Add_None)                                                                                    \
    X(ScPlayer, HAS_ScPlayer, Add_None)                                                                                \
    X(ScRemote, HAS_ScRemote, Add_None)                                                                                \
    X(ScAi, HAS_ScAi, Add_None)                                                                                        \
    X(ScAilearn, HAS_ScAilearn, Add_None)                                                                              \
    X(ScBody2, HAS_ScBody2, Add_None)                                                                                  \
    X(ScBody3, HAS_ScBody3, Add_None)                                                                                  \
    X(ScStage, HAS_ScStage, Add_None)                                                                                  \
    X(ScModel, HAS_ScModel, Add_None)                                                                                  \
    X(ScAnim, HAS_ScAnim, Add_None)                                                                                    \
    X(ScCamera, HAS_ScCamera, Add_None)                                                                                \
    X(ScInteract, HAS_ScInteract, Add_None)                                                                            \
    X(ScMove3, HAS_ScMove3, Add_None)                                                                                  \
    X(ScMove2, HAS_ScMove2, Add_None)                                                                                  \
    X(ScAbility, HAS_ScAbility, Add_None)                                                                              \
    X(ScBuff, HAS_ScBuff, Add_None)                                                                                    \
    X(ScTimer, HAS_ScTimer, Add_None)                                                                                  \
    X(ScAudio, HAS_ScAudio, Add_None)                                                                                  \
    X(ScParent, HAS_ScParent, ScParent_Init)                                                                                \
    X(ScOwner, HAS_ScOwner, Add_None)                                                                                  \
    X(ScCombat, HAS_ScCombat, Add_None)                                                                                \
    X(ScReplication, HAS_ScReplication, Add_None)                                                                      \
    X(ScEmitter, HAS_ScEmitter, Add_None)                                                                              \
    X(ScSlider, HAS_ScSlider, Add_None)                                                                                \
    X(ScView2, HAS_ScView2, Add_None)                                                                                  \
    X(ScView3, HAS_ScView3, Add_None)                                                                                  \
    X(ScProjectile, HAS_ScProjectile, Add_None)                                                                        \
    X(ScHudslot, HAS_ScHudslot, Add_None)                                                                              \
    X(ScHuditem, HAS_ScHuditem, Add_None)                                                                              \
    X(ScTooltip, HAS_ScTooltip, Add_None)                                                                              \
    X(ScZone, HAS_ScZone, Add_None)                                                                                    \
    X(ScRef, HAS_ScRef, Add_None)                                                                                      \
    X(ScAbilitybar, HAS_ScAbilitybar, Add_None)                                                                        \
    X(ScRibbon, HAS_ScRibbon, Add_None)                                                                                \
    X(ScWeapon, HAS_ScWeapon, Add_None)                                                                                \
    X(ScInventory, HAS_ScInventory, ScInventory_Init)                                                                  \
    X(ScUser, HAS_ScUser, Add_None)                                                                                    \
    X(ScEquip, HAS_ScEquip, Add_None)                                                                                  \
    X(ScUiItem, HAS_ScUiItem, Add_None)                                                                                \
    X(ScScore, HAS_ScScore, Add_None)                                                                                  \
    X(ScBuilder, HAS_ScBuilder, Add_None)

typedef enum
{
#define AS_ENUM(Type, Flag, ...) Flag,
    SOL_COMPONENT_LIST(AS_ENUM)
#undef AS_ENUM
    COMPONENT_COUNT
} SolComponents;

#define COMP_INIT_FWD(Type, Flag, InitFn) void InitFn(World *world, int id, void *self);
SOL_COMPONENT_LIST(COMP_INIT_FWD)
#undef COMP_INIT_FWD

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
#define DECLARE_SPARSE_STRUCTS(type, ...) SPARSE_SET_STRUCT(type);
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

// set->, i , c->
#define for_set(w, type)                                                                                               \
    for (SparseSet_##type *set = Sol_Comp_Set(w, type); set; set = NULL)                                               \
        for (int i = 0; i < set->cnt; i++)                                                                             \
            for (type *c = &set->data[i]; c; c = NULL)