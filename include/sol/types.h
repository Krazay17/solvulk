#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

#include <cglm/types-struct.h>

#ifdef SOL_VULK_SHARED
    #ifdef SOL_BUILD_DLL
        #define SOLAPI __declspec(dllexport)
    #else
        #define SOLAPI __declspec(dllimport)
    #endif
#else
    #define SOLAPI 
#endif

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

// Forwards
typedef struct World World;
typedef struct SolState SolState;
typedef struct SolModel SolModel;
typedef struct SolCamera SolCamera;

// Defs
typedef void (*InteractCallback)(void *data);
typedef bool Active;
typedef uint32_t Mask;

// Enums
typedef enum
{
    ACTION_NONE = 0,
    ACTION_FWD = (1 << 0),
    ACTION_BWD = (1 << 1),
    ACTION_LEFT = (1 << 2),
    ACTION_RIGHT = (1 << 3),
    ACTION_JUMP = (1 << 4),
    ACTION_DASH = (1 << 5),
} PlayerActionStates;

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
    MOVE_CONFIG_PLAYER,
    MOVE_CONFIG_WIZARD,
    MOVE_CONFIG_COUNT,
} MoveConfigId;

typedef enum
{
    MOVE_IDLE,
    MOVE_WALK,
    MOVE_FALL,
    MOVE_JUMP,
    MOVE_DASH,
    MOVE_SLIDE,
    MOVE_FLY,
    MOVE_STATE_COUNT
} MoveState;

typedef enum
{
    SOL_MODEL_WIZARD,
    SOL_MODEL_WORLD0,
    SOL_MODEL_WORLD1,
    SOL_MODEL_WORLD2,
    SOL_MODEL_COUNT,
} SolModelId;

typedef enum
{
    SOL_FONT_ICE,
    SOL_FONT_COUNT,
} SolFontId;

typedef enum
{
    SOL_TEX_REDSKY,
    SOL_TEX_COUNT,
} SolTexId;

typedef enum
{
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

// clang-format off
typedef enum
{
    HAS_NONE                =   0,
    HAS_XFORM               =   (1 << 0),
    HAS_BODY2               =   (1 << 1),
    HAS_BODY3               =   (1 << 2),
    HAS_SHAPE               =   (1 << 4),
    HAS_INTERACT            =   (1 << 5),
    HAS_MODEL               =   (1 << 6),
    HAS_INFO                =   (1 << 7),
    HAS_UI_ELEMENT          =   (1 << 8),
    HAS_MOVEMENT            =   (1 << 9),
    HAS_CONTROLLER          =   (1 << 10),
    HAS_CAMERA              =   (1 << 11),
    HAS_CONTROLLER_AI       =   (1 << 12),
} CompBits;
// clang-format on

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

// Structs
typedef struct
{
    float x, y, z;
} SolVec3;

typedef struct
{
    float x, y;
} SolVec2;

typedef struct
{
    float x, y, w, h;
} SolRect;

typedef struct
{
    uint8_t r, g, b, a;
} SolColor;

typedef struct CompXform
{
    vec3s pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s scale, lastScale, drawScale;
} CompXform;

typedef struct CompBody
{
    vec3s vel, impulse, force;
    Shape3 shape;
    float grounded, airtime;
    float radius, height, length;
    float mass, invMass, restitution;
} CompBody;

typedef struct CompShape
{
    Shape2 type;
    float width, height;
} CompShape;

typedef struct CompInteractable
{
    bool isHovered, isPressed, isClicked, onHold;
    InteractCallback callback;
    void *callbackData;
} CompInteractable;

typedef struct CompModel
{
    uint32_t gpuHandle;
    SolModel *model;
    float yOffset;
} CompModel;

typedef struct CompInfo
{
    char name[24];

} CompInfo;

typedef struct CompUiElement
{
    SolColor baseColor;
    SolColor textColor;
    float fontSize;
    char text[64];
    float textWidth;

    float hoverAnim;
    float clickAnim;

    float borderThickness;
    SolColor borderColor;
} CompUiElement;

typedef struct CompMovement
{
    vec3s wishdir, updir, lockdir;
    float stateTimer;
    MoveState moveState;
    MoveConfigId configId;
} CompMovement;

typedef struct CompController
{
    vec3s lookdir, wishdir;
    vec2s wishdir2;
    uint32_t actionState;
    float yaw, pitch;
} CompController;

typedef struct AiController
{
    vec3s lookdir, wishdir;
    AiAction actionState;
} AiController;

typedef struct
{
    int x, y;
    int dx, dy;
    bool locked;
    bool buttons[SOL_MOUSE_COUNT];
    bool buttonsPressed[SOL_MOUSE_COUNT];
} SolMouse;
