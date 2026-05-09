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
#define WORLD_FORWARD (vec3s){0, 0, -1.0f}
#define WORLD_UP (vec3s){0, 1.0f, 0}
#define WORLD_DOWN (vec3s){0, -1.0f, 0}

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define TARGET_ASPECT 16.0f / 9.0f

#define ColorConvert(x) (x / 255.0f)
#define UISCALE(x) (x * min(Sol_GetState()->windowWidth / WINDOW_WIDTH, Sol_GetState()->windowHeight / WINDOW_HEIGHT))

// Forwards
typedef struct World     World;
typedef struct SolState  SolState;
typedef struct SolCamera SolCamera;

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
    STRAFE_LEFT,
    STRAFE_BWD,
    STRAFE_RIGHT,
} StrafeDir;

typedef enum
{
    AI_IDLE,
    AI_FWD,
    AI_BWD,
    AI_LEFT,
    AI_RIGHT,
    AI_JUMP,
    AI_DASH,
} AiAction;

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

typedef enum
{
    SOL_TEX_REDSKY,
    SOL_TEX_COUNT,
} SolTexId;

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
    SOL_KEY_SPACE,
    SOL_KEY_ESCAPE,
    SOL_KEY_SHIFT,
    SOL_KEY_COUNT
} SolKey;

typedef enum
{
    SOL_MOUSE_LEFT,
    SOL_MOUSE_RIGHT,
    SOL_MOUSE_MIDDLE,
    SOL_MOUSE_COUNT
} SolMouseButton;

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

// Structs ----------------------------------------------------

typedef void (*CallbackFunc)(int flags, void *);
typedef struct
{
    CallbackFunc callbackFunc;
    void        *callbackData;
} Callback;

// ─── Font data ───────────────────────────────────────────────────

typedef struct AiController
{
    vec3s    lookdir, wishdir;
    AiAction actionState;
} AiController;

typedef struct SolMouse
{
    int  x, y;
    int  dx, dy;
    int  wheelV;
    bool locked;
    bool buttons[SOL_MOUSE_COUNT];
    bool buttonsPressed[SOL_MOUSE_COUNT];
} SolMouse;

typedef struct SolRay
{
    vec3s pos, dir;
    float dist;
    float min;
    u8    mask;
    u32   ignoreEnt;
} SolRay;

typedef struct SolHit
{
    vec3s pos, dir;
    float power;
    u32   damage;
} SolHit;

typedef struct SolLook
{
    float yaw, pitch, sens;
    vec3s lookdir;
} SolLook;

typedef struct SolRayResult
{
    bool  hit;
    vec3s pos, norm;
    float dist;
    u32   triIndex;
    u32   entId;
} SolRayResult;

typedef struct SolCamera
{
    vec3  position;
    vec3  target;
    float fov;
    float nearClip;
    float farClip;
    mat4  proj;
    mat4  view;
    mat4  viewProj;
} SolCamera;

typedef struct SolLine
{
    vec3s a, b;
    vec4s aColor, bColor;
    float ttl;
} SolLine;

typedef struct SolSphere
{
    vec3s pos;
    vec4s color;
    float radius;
} SolSphere;

typedef enum
{
    EVENT_NONE,
    EVENT_PARTICLE,
    EVENT_COLLISION,
    EVENT_COUNT,
} EventKind;

typedef enum
{
    PARTICLE_ORB,
    PARTICLE_COUNT,
} ParticleKind;

typedef enum
{
    EMITTER_FOUNTAIN,
    EMITTER_COUNT,
} EmitterKind;

typedef struct Particle
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        ttl, scale, span;
} Particle;

typedef struct Emitter
{
    EmitterKind emitterKind;
    vec3s       pos, vel;
    float       ttl, rate, accumulator;
    Particle    particle;
    u32         burst;
} Emitter;

typedef struct SolMaterial
{
    float baseColor[4];
    float emissive[4];
    float metallic;
    float roughness;
} SolMaterial;

typedef enum
{
    SOL_FONT_ICE,
    SOL_FONT_COUNT,
} SolFontKind;

typedef struct
{
    float u, v, uw, vh;
    float xoffset;
    float ytop;
    float yoffset;
    float yadvance;
} SolGlyph;

typedef struct
{
    float l, b, r, t;
} TextBounds;

typedef struct SolFont
{
    SolGlyph   glyph[128];
    TextBounds bounds;
} SolFont;

typedef struct
{
    const char *str;
    float       x, y, size;
    vec4s       color;
    SolFontKind kind;
} SolFontDesc;

typedef enum
{
    SOL_IMAGE_FONT,
    SOL_IMAGE_COUNT,
} SolImageId;

typedef struct SolImage
{
    u32         width, height;
    const void *pixels;
} SolImage;

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

// ___________________________SKELETON___________________

typedef struct SolTri
{
    int   entId;
    vec3s a, b, c;
    vec3s normal, center;
    float bounds;
} SolTri;

typedef struct SolEvent
{
    u32   kind;
    u32   entA, entB;
    vec3s pos, normal;
} SolEvent;

typedef struct SolEvents
{
    SolEvent *event;

    u32 count;
    u32 capacity;
} SolEvents;

typedef struct
{
    float x, y, w, h;
    float u, v, uw, vh;
    float r, g, b, a;
} ShaderPushText;

typedef struct
{
    ShaderPushText *push;
    u32             count;
} ShaderPushTexts;

typedef enum
{
    ABILITY_STATE_IDLE,
    ABILITY_STATE_DASH,
    ABILITY_STATE_CLAW,
    ABILITY_STATE_2,
    ABILITY_STATE_3,
    ABILITY_STATE_4,
    ABILITY_STATE_5,
    ABILITY_STATE_6,
    ABILITY_STATE_7,
    ABILITY_STATE_8,
    ABILITY_STATE_9,
    ABILITY_STATE_COUNT,
} AbilityState;

typedef enum
{
    INTERACT_HOVERED    = (1 << 0),
    INTERACT_PRESSED    = (1 << 1),
    INTERACT_CLICKED    = (1 << 2),
    INTERACT_TOGGLED    = (1 << 3),
    INTERACT_TOGGLEABLE = (1 << 4),
} InteractState;

typedef enum
{
    ANIM_LAYER_BASE,  // full body, always active
    ANIM_LAYER_LOWER, // overrides legs
    ANIM_LAYER_UPPER, // overrides torso/arms
    ANIM_LAYER_OVERRIDE,
    ANIM_LAYER_COUNT
} AnimLayerId;

typedef struct
{
    float       blendIn, blendOut, seek;
    SolAnims    anim;
    AnimLayerId layerId;
    bool        force;
} AnimDesc;