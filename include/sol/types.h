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

typedef struct World World;
typedef struct SolState SolState;
typedef struct SolModel SolModel;
typedef struct SolCamera SolCamera;


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

typedef struct
{
    int x, y;
    int dx, dy;
    bool locked;
    bool buttons[SOL_MOUSE_COUNT];
    bool buttonsPressed[SOL_MOUSE_COUNT];
} SolMouse;