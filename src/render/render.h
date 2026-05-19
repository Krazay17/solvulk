#pragma once
#include "sol/types.h"

#include "font/font.h"

#define MAX_MODEL_INSTANCES (1 << 14)
#define MAX_BILLBOARD_INSTANCES (1 << 20)
#define MAX_SPHERE_INSTANCES (1 << 20)
#define MAX_QUAD_INSTANCES (1 << 20)
#define MAX_LINE_VERTICES 0xffffff

typedef struct SolCamera SolCamera;

typedef enum
{
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,

    PIPE_TEXT,
    PIPE_RECT,
    PIPE_LINE,

    PIPE_SPHERE,
    PIPE_SPHERE_FX,
    PIPE_SPRITE,
    PIPE_SPRITE_FX,

    PIPE_SKYBOX,
    PIPE_BILLBOARD,

    PIPE_COUNT,
} PipelineId;

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

typedef struct
{
    mat4 ortho2d;
} OrthoUBO;

typedef struct
{
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} SceneUBO;

typedef struct
{
    vec4 rec;
    vec4 color;
    vec4 extras;
} ShaderPushRect;

typedef struct
{
    vec4 position;
    vec4 scale;
    vec4 rotation;
    vec4 color;
    vec4 material;
} ModelSSBO;

typedef struct
{
    mat4 bones[MAX_BONES];
} BonesSSBO;

typedef struct
{
    vec4 pos;         // xyz = world position, w = size (uniform scale)
    vec4 color;       // base tint
    vec4 params;      // type-specific (fill amount, glow strength, icon index, …)
    u32  type;        // BILLBOARD_SPHERE, BILLBOARD_HEALTHBAR, …
    u32  _padding[3]; // pad to 16 bytes for std430 alignment
} BillboardSSBO;

typedef struct
{
    vec4 pos;
    vec4 color;
    vec4 params;
} SphereSSBO;

typedef struct
{
    vec4 pos;      // xyz + size
    vec4 rotation; // quaternion
    vec4 color;    // tint
    vec4 uv;       // atlas UV offset/scale (xy = offset, zw = size)
    u32  type;
    u32  textureId;
    u32  _pad[2];
} QuadSSBO;

typedef struct
{
    u32 flags;
} FlagsSSBO;

typedef struct SolLineVertex
{
    vec3s pos;
    vec4s color;
} SolLineVertex;

typedef struct
{
    vec4s pos;
    vec4s color;
    vec4s params;
    bool  isfx;
} SphereDesc;

typedef enum
{
    BILLBOARD_HEALTHBAR,
    BILLBOARD_ICON,
    BILLBOARD_DAMAGE_NUMBER,
} BillboardKind;
typedef struct
{
    BillboardKind kind;
    vec4s         pos;
    vec4s         color;
    vec4s         params;
    u32           flags;
} BillboardDesc;

typedef enum
{
    SPRITE_CAMFACE,
    SPRITE_3D,
} SpriteKind;
typedef struct
{
    vec4s        pos;
    versors      rotation;
    vec4s        color;
    vec4s        uv;
    SpriteKind   kind;
    SolTextureId textureId;
    bool         isfx;
} SpriteDesc;

typedef struct ModelPushDesc
{
    SolModelId handle;
    vec4s      position;
    vec4s      scale;
    vec4s      rotation;
    vec4s      color;
    vec4s      material;
    u32        flags;
    bool       hasAnim;
    mat4      *bones;
} ModelPushDesc;

typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_SPHERE_INSTANCES];
} SphereQueue;

extern SphereQueue sphereQueue;
extern SphereQueue sphereFxQueue;
typedef struct
{
    u32      count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} SpriteQueue;

extern SpriteQueue spriteQueue;
extern SpriteQueue spriteFxQueue;

void Sol_Render_Camera_Update(SolCamera *cam);
void Sol_Render_Resize(uint32_t width, uint32_t height);
void Sol_Render_Flush3D(void);
void Sol_Render_Flush2D(void);

void Sol_Render_PushSphere(SphereDesc desc);
void Sol_Render_PushBillboard(BillboardDesc desc);
void Sol_Render_PushSprite(SpriteDesc desc);
void Sol_Render_PushModel(ModelPushDesc desc);

float Sol_Render_GetAspect(void);
void  Sol_Render_DrawSkybox(void);
void  Sol_Render_DrawLine(SolLine *lines, int count);
void  Sol_Render_DrawRectangle(vec4s rect, vec4s color, float thickness);
void  Sol_Render_DrawText(SolFontDesc desc);

void Sol_Render_SkyboxSet(World *world, SolTextureId textureId);

static inline SphereSSBO *Sol_Render_GetNext_Sphere(bool isfx)
{
    SphereQueue *q = isfx ? &sphereFxQueue : &sphereQueue;

    if (q->count >= MAX_SPHERE_INSTANCES)
        return NULL;

    return &q->instances[q->count++];
}

static inline QuadSSBO *Sol_Render_GetNext_Sprite(bool isfx)
{
    SpriteQueue *q = isfx ? &spriteFxQueue : &spriteQueue;

    if (q->count >= MAX_SPHERE_INSTANCES)
        return NULL;

    return &q->instances[q->count++];
}