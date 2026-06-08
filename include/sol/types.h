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

#define SOL_PHYS_GRAV (vec3s){0.0f, -9.81f, 0.0f}
#define WORLD_FORWARD (vec3s){0, 0, 1.0f}
#define WORLD_UP (vec3s){0, 1.0f, 0}
#define WORLD_DOWN (vec3s){0, -1.0f, 0}

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define TARGET_ASPECT 16.0f / 9.0f
#define MAX_BONES 128
#define MAX_ENTS (1 << 15)
// Forwards
typedef struct World    World;
typedef struct SolState SolState;

// Enums
typedef enum
{
    ACTION_NONE     = 0,
    ACTION_ABILITY1 = (1 << 0),
    ACTION_ABILITY2 = (1 << 1),
    ACTION_ABILITY3 = (1 << 2),
    ACTION_ABILITY4 = (1 << 3),
    ACTION_ABILITY5 = (1 << 4),
    ACTION_ABILITY6 = (1 << 5),
    ACTION_ABILITY7 = (1 << 6),
    ACTION_ABILITY8 = (1 << 7),
    ACTION_ABILITY9 = (1 << 8),
    ACTION_DASH     = (1 << 10),

    ACTION_FWD    = (1 << 11),
    ACTION_BWD    = (1 << 12),
    ACTION_LEFT   = (1 << 13),
    ACTION_RIGHT  = (1 << 14),
    ACTION_JUMP   = (1 << 15),
    ACTION_CROUCH = (1 << 16),
} SolActions;

typedef enum
{
    COLLISIONGROUP_NONE,
    COLLISIONGROUP_WORLD      = (1 << 0),
    COLLISIONGROUP_PAWN       = (1 << 1),
    COLLISIONGROUP_PROJECTILE = (1 << 2),
} CollisionGroup;

typedef enum Shape3
{
    SHAPE3_SPH,
    SHAPE3_CAP,
    SHAPE3_BOX,
    SHAPE3_MOD,
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
    FXKIND_FIREBALL_SHOOT,
    FXKIND_FIREBALL_HIT,
    FXKIND_FIRE_APPLY,
    FXKIND_SHIELD_BURST,
    FXKIND_SHIELD_HIT,
    FXKIND_SPINHIT,
    FXKIND_BULLET_HIT,
    FXKIND_DEATH_BLOOD,
} FxKind;

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
    FRAMEBUFFER_LINE,
    FRAMEBUFFER_COUNT,
} FrameBufferId;

typedef enum
{
    MODELKIND_WIZARD,
    MODELKIND_DUDE,
    SOL_MODEL_BOX,
    SOL_MODEL_WORLD0,
    SOL_MODEL_WORLD1,
    SOL_MODEL_WORLD2,
    SOL_MODEL_COUNT,
} SolModelKind;

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

typedef struct SolXform
{
    vec3s   pos, rot, scale;
    versors quat;
} SolXform;

typedef void (*CallbackFunc)(int flags, void *);
typedef struct
{
    CallbackFunc callbackFunc;
    void        *callbackData;
} Callback;

// ─── Font data ───────────────────────────────────────────────────

typedef struct SolLine
{
    vec3s a, b;
    vec4s aColor, bColor;
    float ttl;
} SolLine;

typedef enum
{
    EMITTER_FOUNTAIN,
    EMITTER_COUNT,
} EmitterKind;

typedef struct SolMaterial
{
    float baseColor[4];
    float emissive[4];
    float metallic;
    float roughness;
} SolMaterial;

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
    ANIM_FALL,
    ANIM_DASH_FWD,
    ANIM_DASH_BWD,
    ANIM_DASH_LEFT,
    ANIM_DASH_RIGHT,
    ANIM_CHARGE_LEFT,
    ANIM_CHARGE_RIGHT,
    ANIM_ATTACK_LEFT,
    ANIM_ATTACK_RIGHT,
    ANIM_DEATH,
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
    int   entId;
    vec3s a, b, c;
    vec3s normal, center;
    float bounds;
} SolTri;

typedef enum
{
    INTERACT_HOVERED    = (1 << 0),
    INTERACT_PRESSED    = (1 << 1),
    INTERACT_CLICKED    = (1 << 2),
    INTERACT_TOGGLED    = (1 << 3),
    INTERACT_TOGGLEABLE = (1 << 4),
    INTERACT_MOVING     = (1 << 5),
} InteractState;

typedef enum
{
    SOL_TEXTURE_ICEFONT,
    SOL_TEXTURE_FIREPARTICLE,
    SOL_TEXTURE_SHOCKPARTICLE,
    SOL_TEXTURE_CLOUDPARTICLE,
    SOL_TEXTURE_BLOODPARTICLE,
    SOL_TEXTURE_REDSKY,
    SOL_TEXTURE_FIREBALL_CARD,
    SOL_TEXTURE_PISTOL_CARD,
    SOL_TEXTURE_BLADE_CARD,
    SOL_TEXTURE_CRYSTAL_CARD,
    SOL_TEXTURE_DASH_CARD,
    SOL_TEXTURE_CLOUD1,
    SOL_TEXTURE_HEALTH,
    SOL_TEXTURE_SPIKEFRAMEFILLED,
    SOL_TEXTURE_SWIRLFRAME,
    SOL_TEXTURE_CLOUD2,
    SOL_TEXTURE_BORDER,
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
    UILAYER_3,
    UILAYER_4,
    UILAYER_5,
    UILAYER_6,
    UILAYER_7,
    UILAYER_COUNT,
} UiLayer;