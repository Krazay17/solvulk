/*
 * File: render.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 *
 */
#pragma once
#include "sol/types.h"

#include "font.h"
#include "model.h"

#define MAX_MODEL_INSTANCES 0x4fff
#define MAX_RECT_INSTANCES (1 << 14)
#define MAX_FONT_INSTANCES (1 << 16)
#define MAX_QUAD_INSTANCES (1 << 20)
#define MAX_BUFFER_VERTS 0xffffff

typedef struct ScModelData ScModelData;

typedef enum
{
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,

    PIPE_TEXT,
    PIPE_TEXT_3D,
    PIPE_TEXT_3D_FRONT,
    PIPE_TEXT_2D,
    PIPE_RECT,
    PIPE_LINE,

    PIPE_DEBUG_SPHERE,

    PIPE_SPHERE,
    PIPE_SPHERE_FX,
    PIPE_FIREBALL,

    PIPE_SPRITE,
    PIPE_SPRITE_ADD,
    PIPE_SPRITE_FRONT,
    PIPE_HEALTHBAR,
    PIPE_RIBBON,
    PIPE_RIBBON_ADD,
    PIPE_RIBBON_FRONT,

    PIPE_SKYBOX,

    PIPE_COUNT,
} PipelineId;

typedef struct ViewSSBO
{
    mat4s proj;
    mat4s view;
    mat4s viewProj;
    vec3s pos;
    vec3s target, dir;
    vec3s up, right;
    float fov;
    float nearClip;
    float farClip;
    float roll;
} ViewSSBO;

extern ViewSSBO g_solView;

typedef struct
{
    double gameTime;
} GameUtilUBO;

typedef struct
{
    mat4s ortho2d;
} OrthoUBO;

typedef struct RenderVert
{
    vec3s pos;
    vec4s color;
} RenderVert;

typedef struct
{
    float x, y, w, h;
    float u, v, uw, vh;
    float r, g, b, a;
} ShaderPushText;

typedef struct
{
    ShaderPushText *push;
    u32 count;
} ShaderPushTexts;

typedef struct
{
    Rect rect;
    float scale, zindex, spin, fill;
    vec4s color, uv;
    float border;
    // 1 fill vertical, 2 invert fill
    u32 flags;
    u32 textureID;
    u32 _pad;
} RectSSBO;
typedef struct
{
    u32 count;
    RectSSBO instances[MAX_RECT_INSTANCES];
} RectQueue;
extern RectQueue rectQueue[];
static inline RectSSBO *Sol_Render_GetNext_Rect(u32 layer)
{
    assert(rectQueue[layer].count < MAX_RECT_INSTANCES && "rectQueue[layer] Full");
    RectSSBO *ssbo = &rectQueue[layer].instances[rectQueue[layer].count++];
    *ssbo          = (RectSSBO){0};
    return ssbo;
}

typedef struct
{
    vec4s pos, color, uv, outline;
} FontSSBO;
typedef struct
{
    u32 count;
    FontSSBO instances[MAX_FONT_INSTANCES];
} FontQueue;
extern FontQueue font2dQueue[];
static inline FontSSBO *Sol_Render_GetNext_Font(u32 layer)
{
    assert(layer >= 0 && layer < UILAYER_COUNT && "INVALID FONT LAYER");
    assert(font2dQueue[layer].count < MAX_FONT_INSTANCES && "font2dQueue[layer] Full");
    return &font2dQueue[layer].instances[font2dQueue[layer].count++];
}

// typedef enum
// {
//     SPRITE_CAMFACE,
//     SPRITE_3D,
// } SpriteKind;
// typedef struct
// {
//     vec4s pos;
//     versors rotation;
//     vec4s color;
//     vec4s uv;
//     SpriteKind kind;
//     SolTextureId textureId;
//     bool isfx;
// } SpriteDesc;

// typedef struct ModelPushDesc
// {
//     ModelKind handle;
//     vec4s position;
//     vec4s scale;
//     vec4s rotation;
//     vec4s color;
//     vec4s material;
//     u32 flags;
//     bool hasAnim;
//     mat4 *bones;
// } ModelPushDesc;

typedef struct
{
    vec4s position;
    vec4s scale;
    vec4s rotation;
    vec4s color;
    vec4s material;
    u32 flags;
    float hitTime;
    u32 _padding[2];
} ModelSSBO;

typedef struct
{
    u32 count;
    ModelSSBO modelSSBO[MAX_MODEL_INSTANCES];
    ModelKind handles[MAX_MODEL_INSTANCES];
} ModelSubmission;

typedef struct
{
    u32 count;
    ModelSSBO modelSSBO[MAX_MODEL_INSTANCES];
    const SolPose *bones[MAX_MODEL_INSTANCES];
    ModelKind handles[MAX_MODEL_INSTANCES];
} ModelSkinnedSubmission;

extern ModelSubmission modelQueue;
extern ModelSkinnedSubmission skinningQueue;

static inline void Sol_Render_GetNext_Model(ModelKind handle, ModelSSBO *modelSSBO, SolPose *pose)
{
    if (pose)
    {
        if (skinningQueue.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                      = skinningQueue.count++;
        skinningQueue.handles[idx]   = handle;
        skinningQueue.bones[idx]     = pose;
        skinningQueue.modelSSBO[idx] = *modelSSBO;
    }
    else
    {
        if (modelQueue.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                   = modelQueue.count++;
        modelQueue.handles[idx]   = handle;
        modelQueue.modelSSBO[idx] = *modelSSBO;
    }
}

// SPHERES ######################

typedef struct SphereSSBO
{
    vec4s pos;
    vec4s color;
    vec4s extra;
} SphereSSBO;

typedef enum
{
    SPHEREKIND_BASIC,
    SPHEREKIND_BASICFX,
    SPHEREKIND_FIREBALL,
    SPHEREKIND_DEBUG,
    SPHEREKIND_COUNT,
} SphereKind;

typedef struct
{
    SphereSSBO instances[MAX_QUAD_INSTANCES];
    u32 count;
} SphereQueue;

extern SphereQueue sphereQueues[SPHEREKIND_COUNT];
static inline SphereSSBO *Sol_Render_GetNextSphere(SphereKind kind)
{
    SphereQueue *q = &sphereQueues[kind];
    if (q->count >= MAX_QUAD_INSTANCES)
        return NULL;
    SphereSSBO *ssbo = &q->instances[q->count++];
    *ssbo            = (SphereSSBO){0};

    return ssbo;
}

// RIBBONS #########################

typedef struct
{
    vec4s posA; // .xyz = world pos, .w = half-width
    vec4s posB; // .xyz = world pos, .w = half-width
    vec4s colorA;
    vec4s colorB;
    vec4s uv;
    u32 textureId, _pad0, _pad1, _pad2;
} RibbonSegSSBO;

#define MAX_RIBBON_SEGS_TOTAL (1 << 16)

typedef struct
{
    u32 count;
    RibbonSegSSBO instances[MAX_RIBBON_SEGS_TOTAL];
} RibbonQueue;

extern RibbonQueue ribbonQueue;
extern RibbonQueue ribbonQueueAdd;
extern RibbonQueue ribbonQueueFront;

static inline RibbonSegSSBO *Sol_Render_GetNext_RibbonSeg(u8 kind)
{
    u32 totalSegCount = ribbonQueue.count + ribbonQueueAdd.count + ribbonQueueFront.count;
    if (totalSegCount >= MAX_RIBBON_SEGS_TOTAL)
        return NULL;
    switch (kind)
    {
    case 0:
        return &ribbonQueue.instances[ribbonQueue.count++];
    case 1:
        return &ribbonQueueFront.instances[ribbonQueueFront.count++];
    case 2:
        return &ribbonQueueAdd.instances[ribbonQueueAdd.count++];
    }
    return NULL;
}

typedef enum
{
    QUADFLAG_NONE,
    QUADFLAG_FILL_VERTICAL,
    QUADFLAG_FILL_INVERT,
} QuadFlags;

typedef enum
{
    QUADKIND_SPRITE,
    QUADKIND_SPRITE_ADD,
    QUADKIND_SPRITE_FRONT,
    QUADKIND_HEALTH,
    QUADKIND_TEXT,
    QUADKIND_TEXT_FRONT,
    QUADKIND_RECT,
    QUADKIND_TEXT2D,
    QUADKIND_COUNT,
} QuadKind;

typedef enum
{
    QUADTYPE_FACECAM,
    QUADTYPE_QUAT,
} QuadType;
typedef struct
{
    vec4s pos, rect, rot, color, uv, extra;
    u32 type, flags, textureId, _pad;
} QuadSSBO;
typedef struct
{
    u32 count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} QuadQueue;

extern QuadQueue quadQueues[QUADKIND_COUNT];
static inline QuadSSBO *Sol_Render_GetNextQuad(QuadKind kind)
{
    if (kind >= QUADKIND_COUNT)
        return NULL;
    QuadQueue *q = &quadQueues[kind];
    if (!q || q->count >= MAX_QUAD_INSTANCES)
        return NULL;
    QuadSSBO *ssbo = &q->instances[q->count++];
    *ssbo          = (QuadSSBO){0};

    return ssbo;
}

typedef struct
{
    mat4s viewProjection;
    mat4s view;
    mat4s proj;
    vec4s cameraPos;
    vec4s sun;
    float aspect;
} SceneUBO;

SceneUBO *Sol_Render_GetNext_Scene();

int Sol_Render_Init(void *hwnd, void *hInstance);

void Sol_Begin_Draw();
void Sol_End_Draw();

void Sol_Render_Resize(uint32_t width, uint32_t height);
void Sol_Render_Flush3D(void);
void Sol_Render_Flush2D(void);
void Sol_Render_CheckGpuUploads();

float Sol_Render_GetAspect(void);
void Sol_Render_DrawSkybox(void);
void Sol_Render_DrawLines(const SolLine *lines, int count, size_t stride);
void Sol_Render_DrawText(const char *str, SolFontDesc desc);
void Sol_Render_UploadImage(u32 width, u32 height, const void *pixels, u32 id, u8 unorm);
void Sol_Render_UploadModel(ScModelData *model, u32 kind);
void Sol_Render_DrawText2D(const char *str, SolFontDesc desc);
void Sol_Render_DrawText3D(const char *str, Text3DDesc desc);