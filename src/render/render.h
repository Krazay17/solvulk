#pragma once
#include "sol/types.h"

#include "font/font.h"

#define MAX_MODEL_INSTANCES (1 << 14)
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
    PIPE_FIREBALL,
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
    double gameTime;
} GameUtilUBO;

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
    vec4  position;
    vec4  scale;
    vec4  rotation;
    vec4  color;
    vec4  material;
    u32   flags;
    float hitTime;
    u32   _padding[2];
} ModelSSBO;

typedef struct
{
    mat4 bones[MAX_BONES];
} BonesSSBO;

typedef struct
{
    vec4 pos;    // xyz = world position, w = size (uniform scale)
    vec4 color;  // base tint
    vec4 params; // type-specific (fill amount, glow strength, icon index, …)
    u32  type;   // BILLBOARD_SPHERE, BILLBOARD_HEALTHBAR, …
    u32  flags;
    u32  _padding[2]; // pad to 16 bytes for std430 alignment
} BillboardSSBO;

typedef struct
{
    vec4 pos;
    vec4 color;
    vec4 params;
    u32  type;
    u32  _padding[3];
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
    ModelSSBO  modelSSBO[MAX_MODEL_INSTANCES];
    SolModelId handles[MAX_MODEL_INSTANCES];
} ModelSubmission;

typedef struct
{
    u32        count;
    ModelSSBO  modelSSBO[MAX_MODEL_INSTANCES];
    BonesSSBO  bones[MAX_MODEL_INSTANCES];
    SolModelId handles[MAX_MODEL_INSTANCES];
} ModelSkinnedSubmission;

extern ModelSubmission        modelQueue;
extern ModelSkinnedSubmission skinningQueue;
static inline void            Sol_Render_GetNext_Model(SolModelId handle, ModelSSBO *modelSSBO, BonesSSBO *bonesSSBO)
{
    // Bounds checking to prevent buffer overflows!
    if (bonesSSBO)
    {
        if (skinningQueue.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                    = skinningQueue.count++;
        skinningQueue.handles[idx] = handle;
        memcpy(&skinningQueue.bones[idx], bonesSSBO, sizeof(BonesSSBO));
        memcpy(&skinningQueue.modelSSBO[idx], modelSSBO, sizeof(ModelSSBO));
    }
    else
    {
        if (modelQueue.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                 = modelQueue.count++;
        modelQueue.handles[idx] = handle;
        memcpy(&modelQueue.modelSSBO[idx], modelSSBO, sizeof(ModelSSBO));
    }
}

typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_QUAD_INSTANCES];
} SphereQueue;
extern SphereQueue        sphereQueue;
extern SphereQueue        sphereFxQueue;
static inline SphereSSBO *Sol_Render_GetNext_Sphere(bool isfx)
{
    SphereQueue *q = isfx ? &sphereFxQueue : &sphereQueue;

    if (q->count >= MAX_QUAD_INSTANCES)
        return NULL;

    return &q->instances[q->count++];
}

typedef struct
{
    u32      count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} SpriteQueue;
extern SpriteQueue      spriteQueue;
extern SpriteQueue      spriteFxQueue;
static inline QuadSSBO *Sol_Render_GetNext_Sprite(bool isfx)
{
    SpriteQueue *q = isfx ? &spriteFxQueue : &spriteQueue;

    if (q->count >= MAX_QUAD_INSTANCES)
        return NULL;

    return &q->instances[q->count++];
}

typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_QUAD_INSTANCES];
} FireballQueue;
extern FireballQueue      fireballQueue;
static inline SphereSSBO *Sol_Render_GetNext_Fireball(void)
{
    FireballQueue *q = &fireballQueue;
    if (q->count >= MAX_QUAD_INSTANCES)
        return NULL;

    return &q->instances[q->count++];
}

void Sol_Begin_Draw();
void Sol_End_Draw();

void Sol_Render_Camera_Update(SolCamera *cam);
void Sol_Render_Resize(uint32_t width, uint32_t height);
void Sol_Render_Flush3D(void);
void Sol_Render_Flush2D(void);

void Sol_Render_PushBillboard(BillboardDesc desc);

float Sol_Render_GetAspect(void);
void  Sol_Render_DrawSkybox(void);
void  Sol_Render_DrawLine(SolLine *lines, int count);
void  Sol_Render_DrawRectangle(vec4s rect, vec4s color, float thickness);
void  Sol_Render_DrawText(SolFontDesc desc);

void Sol_Render_SkyboxSet(World *world, SolTextureId textureId);
