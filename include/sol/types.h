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

// Forwards
typedef struct World    World;
typedef struct SolState SolState;

// Enums
typedef enum
{
    ACTION_NONE  = 0,
    ACTION_FWD   = (1 << 0),
    ACTION_BWD   = (1 << 1),
    ACTION_LEFT  = (1 << 2),
    ACTION_RIGHT = (1 << 3),
    ACTION_JUMP  = (1 << 4),
    ACTION_DASH  = (1 << 5),

    ACTION_ABILITY0 = (1 << 6),
    ACTION_ABILITY1 = (1 << 7),
    ACTION_ABILITY2 = (1 << 8),
    ACTION_ABILITY3 = (1 << 9),
    ACTION_ABILITY4 = (1 << 10),
    ACTION_ABILITY5 = (1 << 11),
    ACTION_ABILITY6 = (1 << 12),
    ACTION_ABILITY7 = (1 << 13),
    ACTION_ABILITY8 = (1 << 14),
    ACTION_ABILITY9 = (1 << 15),
} SolActions;

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
} StrafeDir;

typedef enum
{
    MOVE_IDLE,
    MOVE_WALK,
    MOVE_FALL,
    MOVE_JUMP,
    MOVE_SLIDE,
    MOVE_FLY,
    MOVE_STATE_COUNT
} MoveState;

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
    FXKIND_FIREBALL_HIT,
    FXKIND_FIRE_APPLY,
    FXKIND_SHIELD_BURST,
    FXKIND_SHIELD_HIT,
} FxKind;

typedef enum
{
    MOVE_CONFIG_PLAYER,
    MOVE_CONFIG_WIZARD,
    MOVE_CONFIG_COUNT,
} MoveConfigId;

typedef enum
{
    FRAMEBUFFER_LINE,
    FRAMEBUFFER_COUNT,
} FrameBufferId;

typedef enum
{
    SOL_MODEL_WIZARD,
    SOL_MODEL_DUDE,
    SOL_MODEL_BOX,
    SOL_MODEL_WORLD0,
    SOL_MODEL_WORLD1,
    SOL_MODEL_WORLD2,
    SOL_MODEL_COUNT,
} SolModelId;

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

typedef struct
{
    float radius;
    vec4s color;
} ShapeDesc;

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
    ANIM_IDLE,
    ANIM_WALK_FWD,
    ANIM_WALK_BWD,
    ANIM_WALK_LEFT,
    ANIM_WALK_RIGHT,
    ANIM_JUMP,
    ANIM_FALL,
    ANIM_DASH_FWD,
    ANIM_DASH_BWD,
    ANIM_DASH_LEFT,
    ANIM_DASH_RIGHT,
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
    ANIM_COUNT,
} SolAnims;

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
} InteractState;
