#pragma once
#include "base.h"

#ifdef SOL_VULK_SHARED
#ifdef SOL_BUILD_DLL
#define SOLAPI __declspec(dllexport)
#else
#define SOLAPI __declspec(dllimport)
#endif
#else
#define SOLAPI
#endif

#define WORLD_FORWARD (vec3s){0, 0, 1.0f}
#define WORLD_UP (vec3s){0, 1.0f, 0}
#define WORLD_DOWN (vec3s){0, -1.0f, 0}

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define TARGET_ASPECT 16.0f / 9.0f

#define SOL_TIMESTEP (1.0 / 60.0)
#define MAX_ENTS 0x4fff
#define MAX_BONES 128
#define PHYSXMASK(g, m) ((g << 16) | m)
#define ABILITY_SLOTS 10
#define SOL_PHYS_GRAV (vec3s){0.0f, -9.81f, 0.0f}

typedef void (*SystemFunc)(World *);
typedef void (*SystemFuncId)(World *, int id);
typedef void (*SystemInit)(World *);
typedef void (*SystemDeinit)(World *);
typedef void (*SystemUpdate)(World *, double);
typedef void (*TickEnt)(World *, int, double);
typedef float (*GetterFunc)(World *world, int id);

typedef struct
{
    mat4 bones[MAX_BONES];
} SolPose;

typedef struct
{
    vec3s   poseT[MAX_BONES];
    vec3s   poseS[MAX_BONES];
    versors poseR[MAX_BONES];
} SolPoseE;

typedef enum
{
    EKIND_PLAYER = 1,
    EKIND_WIZARD,
    EKIND_ZORGON,
    EKIND_FIREBALL,
    EKIND_BULLET,
    EKIND_FLOOR,
    EKIND_BOX,
    EKIND_BUTTON,
    EKIND_COUNT,
} EKind;

// Enums
typedef enum
{
    COLLISIONGROUP_NONE,
    COLLISIONGROUP_WORLD      = (1 << 0),
    COLLISIONGROUP_PAWN       = (1 << 1),
    COLLISIONGROUP_PROJECTILE = (1 << 2),
} CollisionGroup;

typedef enum
{
    EVENTKIND_COLLISION,
    EVENTKIND_FX,
    EVENTKIND_SOUND,
    EVENTKIND_EQUIP,
    EVENTKIND_SCORE,
    EVENTKIND_COUNT,
} EventKind;

typedef enum
{
    ANIM_LAYER_BASE,  // full body, always active
    ANIM_LAYER_LOWER, // overrides legs
    ANIM_LAYER_UPPER, // overrides torso/arms
    ANIM_LAYER_OVERRIDE,
    ANIM_LAYER_COUNT
} AnimLayerId;

typedef enum
{
    MOVEMENTKIND_PLAYER,
    MOVEMENTKIND_SPECTATE,
    MOVEMENTKIND_WIZARD,
    MOVEMENTKIND_COUNT,
} MovementKind;

typedef enum
{
    EMITTERKIND_FLASH_BALL,
    EMITTERKIND_FLASH_FIREBALL,
    EMITTERKIND_SINGLE_SPARK,
    EMITTERKIND_BURST_SPARKS,
    EMITTERKIND_BURST_CLOUDS,
    EMITTERKIND_BURST_FIRE,
    EMITTERKIND_POP_FIRE,
    EMITTERKIND_FOUNTAIN_FIRE,
    EMITTERKIND_FOUNTAIN_FOG,
    EMITTERKIND_FOUNTAIN_SPARKS,
    EMITTERKIND_COUNT,
} EmitterKind;

typedef enum
{
    VIEW2DKIND_RECT,
    VIEW2DKIND_TEXT,
    VIEW2DKIND_CIRCLE,
    VIEW2DKIND_COUNT,
} View2dKind;

typedef enum Shape3
{
    SHAPE3_SPH,
    SHAPE3_CAP,
    SHAPE3_BOX,
    SHAPE3_CNT,
} Shape3;

typedef enum Shape2
{
    SHAPE2_CIR,
    SHAPE2_REC,
    SHAPE2_TRI,
    SHAPE2_CNT,
} Shape2;

typedef enum
{
    WALLTOUCH_FRONT,
    WALLTOUCH_LEFT,
    WALLTOUCH_BACK,
    WALLTOUCH_RIGHT,
    WALLTOUCH_COUNT,
} WallTouch;

typedef enum
{
    AISTATE_IDLE,
    AISTATE_PATROL,
    AISTATE_SEARCH,
    AISTATE_AGGRO,
    AISTATE_RETREAT,
    AISTATE_COUNT,
} AiState;
typedef enum
{
    AIKIND_WIZARD,
} AiKind;

typedef enum
{
    MOVE_IDLE,
    MOVE_WALK,
    MOVE_STUN,
    MOVE_FALL,
    MOVE_JUMP,
    MOVE_CROUCH,
    MOVE_SLIDE,
    MOVE_WALLRUN,
    MOVE_WALLJUMP,
    MOVE_MANTLE,
    MOVE_LANDING,
    MOVE_FLY,
    MOVE_DEAD,
    MOVE_STATE_COUNT
} MoveState;

typedef enum
{
    FRAMEBUFFER_LINE,
    FRAMEBUFFER_COUNT,
} FrameBufferId;

typedef enum
{
    SOL_AUDIO_BEEP1,
    SOL_AUDIO_BEEP2,
    SOL_AUDIO_DIGILOAD,
    SOL_AUDIO_HIT,
    SOL_AUDIO_GOTHIT,
    SOL_AUDIO_SWORD_SWING,
    SOL_AUDIO_PARRY,
    SOL_AUDIO_WOODCOCK,
    SOL_AUDIO_LIGHTNINGHIT,
    SOL_AUDIO_LASER,
    SOL_AUDIO_SWORDHIT,
    SOL_AUDIO_MENUMUSIC,
    SOL_AUDIO_SPACEGUN,
    SOL_AUDIO_WOONG,
    SOL_AUDIO_FIREBALL,
    SOL_AUDIO_DASH,
    SOL_AUDIO_FIREBALLIMPACT,
    SOL_AUDIO_COUNT,
} ScAudioId;

typedef enum
{
    TOOLTIPKIND_CARD,
    TOOLTIPKIND_PLAYER_INTERACT,
    TOOLTIPKIND_COUNT,
} TooltipKind;

typedef enum
{
    PARTICLE_ORB,
    PARTICLE_FIRE,
    PARTICLE_SHOCK,
    PARTICLE_SHOCK_ADD,
    PARTICLE_CLOUD,
    PARTICLE_BLOOD,
    PARTICLE_FIREBALL,
    PARTICLE_SPARKFRONT,
    PARTICLE_COUNT,
} ParticleKind;

typedef enum
{
    PROJECTILEKIND_BULLET,
    PROJECTILEKIND_FIREBALL,
    PROJECTILEKIND_COUNT,
} ProjectileKind;

typedef enum
{
    STRAFE_FWD,
    STRAFE_FWD_LEFT,
    STRAFE_LEFT,
    STRAFE_BWD_LEFT,
    STRAFE_BWD,
    STRAFE_BWD_RIGHT,
    STRAFE_RIGHT,
    STRAFE_FWD_RIGHT,
    STRAFE_COUNT,
} StrafeDir;

typedef enum
{
    SOL_QUAD_GFLAME,
    SOL_QUAD_COUNT,
} SolQuadId;

typedef enum
{
    EFLAG_PICKUPABLE = (1 << 0),
    EFLAG_PICKEDUP   = (1 << 1),
    EFLAG_PROJECTILE = (1 << 2),
    EFLAG_HEALTHBAR  = (1 << 3),
} EFlag;

typedef void (*CallbackFunc)(int, void *);
typedef struct
{
    CallbackFunc callbackFunc;
    void        *callbackData;
    int          flag;
} SolCallback;

typedef struct SolLine
{
    vec3s a, b;
    vec4s aColor, bColor;
    float ttl;
} SolLine;

// TEXTURE---------------

typedef enum
{
    ANIM_NONE,
    ANIM_IDLE,
    ANIM_WALK_FWD,
    ANIM_WALK_BWD,
    ANIM_WALK_LEFT,
    ANIM_WALK_RIGHT,
    ANIM_CROUCHWALK_FWD,
    ANIM_CROUCHWALK_BWD,
    ANIM_CROUCHWALK_LEFT,
    ANIM_CROUCHWALK_RIGHT,
    ANIM_SLIDE_FWD,
    ANIM_SLIDE_BWD,
    ANIM_SLIDE_LEFT,
    ANIM_SLIDE_RIGHT,
    ANIM_JUMP,
    ANIM_FLIPJUMP,
    ANIM_FALL,
    ANIM_DASH_FWD,
    ANIM_DASH_BWD,
    ANIM_DASH_LEFT,
    ANIM_DASH_RIGHT,
    ANIM_CHARGE_LEFT,
    ANIM_CHARGE_RIGHT,
    ANIM_CHANNEL_LEFT,
    ANIM_CHANNEL_RIGHT,
    ANIM_WALLJUMP_LEFT,
    ANIM_WALLJUMP_RIGHT,
    ANIM_WALLRUN_FWD,
    ANIM_WALLRUN_LEFT,
    ANIM_WALLRUN_RIGHT,
    ANIM_BACKFLIP,
    ANIM_HARDLAND,
    ANIM_MANTLE,
    ANIM_MANTLE_ROLL,
    ANIM_ATTACK_LEFT,
    ANIM_ATTACK_RIGHT,
    ANIM_DEATH,
    ANIM_STUN,
    ANIM_ABILITY0,
    ANIM_ABILITY1,
    ANIM_ABILITY2,
    ANIM_ABILITY3,
    ANIM_ABILITY4,
    ANIM_ABILITY5,
    ANIM_ABILITY6,
    ANIM_ABILITY7,
    ANIM_ABILITY8,
    ANIM_ABILITY9,
    ANIM_SPINSLASH,
    ANIM_COUNT,
} AnimId;

typedef enum
{
    MODELKIND_WIZARD,
    MODELKIND_DUDE,
    MODELKIND_ZORGON,
    MODELKIND_WEAPONBLADE,
    MODELKIND_WALL,
    MODELKIND_FLOOR,
    MODELKIND_SHIELD,
    MODELKIND_FROSTSWORD,
    MODELKIND_EVAN,
    MODELKIND_EVANRIGGED,
    MODELKIND_WORLD4,
    SOL_MODEL_BOX,
    SOL_MODEL_WORLD0,
    SOL_MODEL_WORLD1,
    SOL_MODEL_WORLD2,
    SOL_MODEL_WORLD6,
    SOL_MODEL_WORLD7,
    SOL_MODEL_WORLD8,
    SOL_MODEL_WORLD9,
    MODELKIND_WORLD10,
    SOL_MODEL_COUNT,
} ModelKind;

typedef struct SolVertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;

    ivec4 boneIndices; // up to 4 bones per vertex (glTF default)
    vec4  boneWeights; // weights, sum = 1.0
} SolVertex;

typedef struct SolTri
{
    union {
        struct
        {
            vec3s a, b, c;
        };
        struct
        {
            vec3s v0, v1, v2;
        };
        vec3s v[3];
    };
    int   entId;
    vec3s normal, center;
    float bounds;
} SolTri;

typedef enum
{
    INTERACT_NONE,
    INTERACT_TOGGLEABLE = (1 << 1),
    INTERACT_TOGGLED    = (1 << 2),
    INTERACT_HOVERED    = (1 << 3),
    INTERACT_HELD       = (1 << 4),
    INTERACT_PRESSED    = (1 << 5),
    INTERACT_DRAGGABLE  = (1 << 6),
    INTERACT_DRAGGING   = (1 << 7),
} InteractState;

typedef enum
{
    SOL_TEXTURE_ICEFONT,
    SOL_TEXTURE_CROSSHAIR,
    SOL_TEXTURE_FIREPARTICLE,
    SOL_TEXTURE_SHOCKPARTICLE,
    SOL_TEXTURE_CLOUDPARTICLE,
    SOL_TEXTURE_BLOODPARTICLE,
    SOL_TEXTURE_REDSKY,
    SOL_TEXTURE_FIREBALL_CARD,
    SOL_TEXTURE_PISTOL_CARD,
    SOL_TEXTURE_BLADE_CARD,
    SOL_TEXTURE_SPIN_CARD,
    SOL_TEXTURE_LIGHTNING,
    SOL_TEXTURE_CRYSTAL_CARD,
    SOL_TEXTURE_DASH_CARD,
    SOL_TEXTURE_CLOUD1,
    SOL_TEXTURE_HEALTH,
    SOL_TEXTURE_SPIKEFRAMEFILLED,
    SOL_TEXTURE_SWIRLFRAME,
    SOL_TEXTURE_CLOUD2,
    SOL_TEXTURE_BORDER,
    SOL_TEXTURE_BEAM,
    SOL_TEXTURE_IMPACT,
    SOL_TEXTURE_LASER_CARD,
    SOL_TEXTURE_FOGSTRIP,
    SOL_TEXTURE_SHIELD,
    SOL_TEXTURE_GRID,
    SOL_TEXTURE_COUNT,
} SolTextureId;

typedef enum
{
    HITKIND_NORMAL,
    HITKIND_BULLET,
    HITKIND_FIRE,
    HITKIND_FIREBALL,
    HITKIND_FIREBALL_EXPLODE,
    HITKIND_ICE,
    HITKIND_SHIELD_PULSE,
    HITKIND_COUNT,
} HitKind;

typedef enum
{
    UILAYER_0,
    UILAYER_1,
    UILAYER_2,
    UILAYER_COUNT,
} UiLayer;

typedef struct AnimDesc
{
    u8          playKind, force;
    float       blendIn, blendOut, seek, speed;
    AnimLayerId layerId;
    int         anim;
} AnimDesc;

typedef struct SolContact
{
    u32   id, idB;
    vec3s pos, normal;
    float penetration;
} SolContact;

typedef struct SolHit
{
    u32   kind;
    int   entA; // Attacker
    int   entB; // Victim
    float damage;
    vec3s pos;
    vec3s normal;
    vec3s vel;
    float power;

    u32 buffMask;
    u32 effectMask;
} SolHit;

typedef struct SolRay
{
    vec3s start, dir;
    float dist;
    u16   mask;
    int   ignoreEnt;
    bool  debug;
} SolRay;

typedef struct SolRayResult
{
    bool  hit;
    vec3s pos, norm;
    float dist;
    int   entId;
} SolRayResult;

typedef struct AnimLayer
{
    u8      playKind;
    int     currentAnim, lastAnim, animId, last_frame_played;
    vec3s   cachedT[MAX_BONES];
    vec3s   cachedS[MAX_BONES];
    versors cachedR[MAX_BONES];
    float   currentSeek, lastSeek;
    float   blendFactor, blendInSpeed; // Internal crossfade between lastAnim -> currentAnim
    float   playRate;
    float   weight;        // Active layer weight [0.0f - 1.0f]
    float   blendOutSpeed; // Rate at which weight decays during fade-out
    bool    isBlendingOut; // Flag indicating layer weight is decaying
    bool    force, hasSnapshot;
} AnimLayer;

typedef struct SolUserHit
{
    int    hoverId, focusId;
    World *hoverWorld, *focusWorld;
    bool   isHoverUi, isFocusUi, isDragging;
    ivec2s pressPos;
} SolUserHit;

typedef enum
{
    EFFECTMASK_KNOCKBACK         = (1 << 0),
    EFFECTMASK_KNOCKBACK_STRONG  = (1 << 1),
    EFFECTMASK_KNOCKUP           = (1 << 2),
    EFFECTMASK_REFLECTPROJECTILE = (1 << 3),
    EFFECTMASK_CHAINLIGHTNING    = (1 << 4),
    EFFECTMASK_HEALONHIT         = (1 << 5),
} EffectMask;

typedef struct
{
    u32 ability, rarity, newAbility, newRarity;
} AbilityBind;

typedef enum
{
    ACTION_NONE,
    ACTION_ABILITY1,
    ACTION_ABILITY2,
    ACTION_ABILITY3,
    ACTION_ABILITY4,
    ACTION_ABILITY5,
    ACTION_ABILITY6,
    ACTION_ABILITY7,
    ACTION_ABILITY8,
    ACTION_ABILITY9,
    ACTION_DASH,
    ACTION_FWD,
    ACTION_BWD,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_JUMP,
    ACTION_CROUCH,
    ACTION_INTERACT,
    ACTION_ZOOMIN,
    ACTION_ZOOMOUT,
    ACTION_BUILD,
    ACTION_DEBUGTELE,
    ACTION_COUNT,
} SolActions;

typedef enum
{
    SOL_KEY_0,
    SOL_KEY_1,
    SOL_KEY_2,
    SOL_KEY_3,
    SOL_KEY_4,
    SOL_KEY_5,
    SOL_KEY_6,
    SOL_KEY_7,
    SOL_KEY_8,
    SOL_KEY_9,

    SOL_KEY_W,
    SOL_KEY_A,
    SOL_KEY_S,
    SOL_KEY_D,
    SOL_KEY_F,
    SOL_KEY_G,
    SOL_KEY_Q,
    SOL_KEY_E,
    SOL_KEY_SPACE,
    SOL_KEY_ESCAPE,
    SOL_KEY_SHIFT,
    SOL_KEY_ALT,
    SOL_KEY_CTRL,
    SOL_KEY_COUNT
} SolKey;

typedef enum
{
    SOL_MOUSE_LEFT,
    SOL_MOUSE_RIGHT,
    SOL_MOUSE_MIDDLE,
    SOL_MOUSE_COUNT
} SolMouseButton;

typedef struct
{
    vec3s pos, vel;
} SolShoot;

typedef enum
{
    BUFFKIND_FIRE,
    BUFFKIND_STUN,
    BUFFKIND_SPEED,
    BUFFKIND_HOT,
    BUFFKIND_INVULN,
    BUFFKIND_COUNT,
} BuffKind;

typedef enum
{
    ABILITY_STATE_IDLE,
    ABILITY_STATE_DASH,
    ABILITY_STATE_FIREBALL,
    ABILITY_STATE_PISTOL,
    ABILITY_STATE_SPINSLASH,
    ABILITY_STATE_CLAW,
    ABILITY_STATE_SHIELD,
    ABILITY_STATE_LASER,
    ABILITY_STATE_WHIP,
    ABILITY_STATE_FIREBALLVOLLEY,
    ABILITY_STATE_COUNT,
} AbilityState;

typedef struct
{
    u32   state, rarity;
    float damage, cooldown, duration, maxpower;
    u32   buffMask;
    u32   effectMask;
} AbilityConfig;

typedef struct SolItem
{
    AbilityConfig ability;
} SolItem;
