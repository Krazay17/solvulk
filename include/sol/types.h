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

#define FLOATING_EPSILON 1e-7f

#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16
#define MAX_MODEL_INSTANCES 500000


// Forwards

typedef struct SolState SolState;
typedef struct SolModel SolModel;
typedef struct SolCamera SolCamera;

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
    ACTION_ATTACK = (1 << 6),
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

typedef struct AiController
{
    vec3s lookdir, wishdir;
    AiAction actionState;
} AiController;

typedef struct SolMouse
{
    int x, y;
    int dx, dy;
    bool locked;
    bool buttons[SOL_MOUSE_COUNT];
    bool buttonsPressed[SOL_MOUSE_COUNT];
} SolMouse;

typedef struct SolRay
{
    bool hit;
    vec3s pos, normal;
    float distance;
    u32 triIdx;
} SolRay;


typedef struct SolCamera
{
    vec3 position;
    vec3 target;
    float fov;
    float nearClip;
    float farClip;
    mat4 proj;
    mat4 view;
} SolCamera;

typedef struct SolVertex
{
    float position[3];
    float normal[3];
    float uv[2];
} SolVertex;

typedef struct SolTri
{
    vec3s a, b, c;
    vec3s normal;
    vec3s center;
    float boundRadius;
} SolTri;

typedef struct SolMaterial
{
    float baseColor[4];
    float metallic;
    float roughness;
} SolMaterial;

typedef struct SolMesh
{
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
    SolMaterial material;
} SolMesh;

typedef struct SolModel
{
    SolVertex *vertices;
    SolMesh *meshes;
    SolTri *tris;
    u32 *indices;

    u32 vertex_count;
    u32 mesh_count;
    u32 tri_count;
    u32 indice_count;
} SolModel;

typedef struct SolCollision
{
    bool didCollide;
    int id;
    vec3s pos, normal, vel;
} SolCollision;
