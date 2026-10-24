/*
 * File: render.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 *
 */
#pragma once
#include "sol/types.h"
#include "sol_buffer.h"

#include "font.h"
#include "model.h"

#define MAX_MODEL_INSTANCES 0xfffu
#define MAX_RECT_INSTANCES 0xfffu
#define MAX_FONT_INSTANCES 0xffffu
#define MAX_QUAD_INSTANCES 0xffffu
#define MAX_BUFFER_VERTS 0xffffu

typedef struct SolModelData SolModelData;

// 1. Define category pipeline lists
#define SOL_MODEL_PIPELINES(X)                                                                                         \
    X(PIPE_MODEL, "shaders/model.vert.spv", "shaders/model.frag.spv")                                                  \
    X(PIPE_MODEL_SKINNED, "shaders/model_skinned.vert.spv", "shaders/model.frag.spv")                                  \
    X(PIPE_MODEL_TRANSPARENT, "shaders/model.vert.spv", "shaders/model.frag.spv")

#define SOL_TEXT_PIPELINES(X) X(PIPE_TEXT, "shaders/text2d.vert.spv", "shaders/text2d.frag.spv")

#define SOL_SPHERE_PIPELINES(X)                                                                                        \
    X(PIPE_SPHERE, "shaders/sphere.vert.spv", "shaders/sphere.frag.spv")                                               \
    X(PIPE_FIREBALL, "shaders/sphere.vert.spv", "shaders/fireball.frag.spv")                                           \
    X(PIPE_SPHERE_FX, "shaders/sphere.vert.spv", "shaders/sphere.frag.spv")

#define SOL_QUAD_PIPELINES(X)                                                                                          \
    X(PIPE_QUAD, "shaders/quad.vert.spv", "shaders/sprite.frag.spv")                                                   \
    X(PIPE_QUAD_ADD, "shaders/quad.vert.spv", "shaders/sprite.frag.spv")                                               \
    X(PIPE_QUAD_FRONT, "shaders/quad.vert.spv", "shaders/sprite.frag.spv")                                             \
    X(PIPE_HEALTHBAR, "shaders/quad.vert.spv", "shaders/healthbar.frag.spv")                                           \
    X(PIPE_TEXT_3D, "shaders/quad.vert.spv", "shaders/text3d.frag.spv")                                                \
    X(PIPE_TEXT_3D_FRONT, "shaders/quad.vert.spv", "shaders/text3d.frag.spv")

#define SOL_RIBBON_PIPELINES(X)                                                                                        \
    X(PIPE_RIBBON, "shaders/ribbon.vert.spv", "shaders/sprite.frag.spv")                                               \
    X(PIPE_RIBBON_ADD, "shaders/ribbon.vert.spv", "shaders/sprite.frag.spv")                                           \
    X(PIPE_RIBBON_FRONT, "shaders/ribbon.vert.spv", "shaders/sprite.frag.spv")

// 2. Compile-time element counter trick
// put 0 so every X_COUNT macro does +1 to it
#define X_COUNT(id, ...) +1
#define PIPE_MODEL_COUNT (0 SOL_MODEL_PIPELINES(X_COUNT))
#define PIPE_TEXT_COUNT (0 SOL_TEXT_PIPELINES(X_COUNT))
#define PIPE_SPHERE_COUNT (0 SOL_SPHERE_PIPELINES(X_COUNT))
#define PIPE_QUAD_COUNT (0 SOL_QUAD_PIPELINES(X_COUNT))
#define PIPE_RIBBON_COUNT (0 SOL_RIBBON_PIPELINES(X_COUNT))

// 3. Enum expansion macro
#define X_ENUM(id, ...) id,

typedef enum
{
    // PIPE_MODEL,
    // PIPE_MODEL_SKINNED,
    // PIPE_MODEL_TRANSPARENT,
    PIPE_SKYBOX,
    PIPE_RECT,
    PIPE_LINE,

    SOL_MODEL_PIPELINES(X_ENUM)
    SOL_TEXT_PIPELINES(X_ENUM) SOL_SPHERE_PIPELINES(X_ENUM) SOL_QUAD_PIPELINES(X_ENUM) SOL_RIBBON_PIPELINES(X_ENUM)

        PIPE_COUNT,

    PIPE_MODEL_START = PIPE_MODEL,
    PIPE_MODEL_END   = PIPE_MODEL_START + PIPE_MODEL_COUNT,

    PIPE_TEXT_START = PIPE_TEXT,
    PIPE_TEXT_END   = PIPE_TEXT_START + PIPE_TEXT_COUNT,

    PIPE_SPHERE_START = PIPE_SPHERE,
    PIPE_SPHERE_END   = PIPE_SPHERE_START + PIPE_SPHERE_COUNT,

    PIPE_QUAD_START = PIPE_QUAD,
    PIPE_QUAD_END   = PIPE_QUAD_START + PIPE_QUAD_COUNT,

    PIPE_RIBBON_START = PIPE_RIBBON,
    PIPE_RIBBON_END   = PIPE_RIBBON_START + PIPE_RIBBON_COUNT,
} PipelineId;

typedef struct SolView
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
} SolView;
extern SolView g_solView;

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

// typedef struct
// {
//     float x, y, w, h;
//     float u, v, uw, vh;
//     float r, g, b, a;
// } ShaderPushText;

// typedef struct
// {
//     ShaderPushText *push;
//     u32 count;
// } ShaderPushTexts;

typedef struct
{
    vec4s pos;     // xy = screen position, z = depth, w = uniform scale
    vec4s rect;    // xy = local pivot offset, zw = dimensions (width, height)
    vec4s color;   // tint / base color
    vec4s uv;      // xy = UV offset, zw = UV scale
    vec4s extra;   // x=border, y=radius, z=desat
    float spin;    // rotation angle in radians
    u32 flags;     // UI flags / state
    u32 textureId; // texture slot index
    u32 _pad;      // explicit std430 16-byte alignment (total: 96 bytes)
} RectSSBO;
typedef struct
{
    u32 count;
    RectSSBO instances[MAX_RECT_INSTANCES];
} RectQueue;
extern RectQueue rectQueue[];
static inline RectSSBO *Sol_Render_GetNext_Rect(u32 layer)
{
    assert(layer >= 0 && layer < UILAYER_COUNT && "INVALID RECT LAYER");
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
    FontSSBO *ssbo = &font2dQueue[layer].instances[font2dQueue[layer].count++];
    *ssbo          = (FontSSBO){0};
    return ssbo;
}

typedef struct
{
    vec4s position; //16
    vec4s scale; // 32
    vec4s rotation; // 48
    vec4s color; // 64
    vec4s color2; // 80
    vec4s material; // 96
    u32 flags; // 100
    float hitTime; // 104
    u32 _padding[6]; // 128
} ModelSSBO;
typedef struct
{
    u32 count;
    ModelKind handles[MAX_MODEL_INSTANCES];
    ModelSSBO instances[MAX_MODEL_INSTANCES];
} ModelQueue;
typedef struct
{
    u32 count;
    ModelKind handles[MAX_MODEL_INSTANCES];
    ModelSSBO instances[MAX_MODEL_INSTANCES];
    const SolPose *bones[MAX_MODEL_INSTANCES];
} ModelQueueSkinned;
extern ModelQueue modelQueue;
extern ModelQueue modelQueueTransparent;
extern ModelQueueSkinned modelQueueSkinning;
// extern ModelQueue modelQueues[PIPE_MODEL_COUNT];
// static inline ModelSSBO *Sol_Render_GetNextModel(u32 pipe, ModelKind handle)
// {
//     u32 idx = pipe - PIPE_MODEL_START;
//     assert(idx < PIPE_MODEL_COUNT);
//     ModelQueue *q = &modelQueues[idx];
//     int count     = q->count;
//     if (count >= MAX_MODEL_INSTANCES)
//         return NULL;

//     q->handles[count] = handle;
//     ModelSSBO *buffer = &q->instances[count];
//     *buffer           = (ModelSSBO){0};
//     q->count++;
//     return buffer;
// }
static inline void Sol_Render_PushModel(ModelKind handle, ModelSSBO *modelSSBO, SolPose *pose, bool transparent)
{
    if (pose)
    {
        if (modelQueueSkinning.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                           = modelQueueSkinning.count++;
        modelQueueSkinning.handles[idx]   = handle;
        modelQueueSkinning.bones[idx]     = pose;
        modelQueueSkinning.instances[idx] = *modelSSBO;
    }
    else if (transparent)
    {
        if (modelQueue.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                              = modelQueueTransparent.count++;
        modelQueueTransparent.handles[idx]   = handle;
        modelQueueTransparent.instances[idx] = *modelSSBO;
    }
    else
    {
        if (modelQueue.count >= MAX_MODEL_INSTANCES)
            return;
        u32 idx                   = modelQueue.count++;
        modelQueue.handles[idx]   = handle;
        modelQueue.instances[idx] = *modelSSBO;
    }
}

// SPHERES ######################

typedef struct SphereSSBO
{
    vec4s pos;
    vec4s color;
    vec4s extra;
} SphereSSBO;

typedef struct
{
    u32 count;
    SphereSSBO instances[MAX_QUAD_INSTANCES];
} SphereQueue;

extern SphereQueue sphereQueues[PIPE_SPHERE_COUNT];
static inline SphereSSBO *Sol_Render_GetNextSphere(PipelineId kind)
{
    u32 idx = kind - PIPE_SPHERE_START;
    assert(idx < PIPE_SPHERE_COUNT);
    SphereQueue *q   = &sphereQueues[idx];
    SphereSSBO *ssbo = &q->instances[q->count++];
    *ssbo            = (SphereSSBO){0};
    return ssbo;
}

// RIBBONS #########################

typedef struct
{
    vec4s posA;
    vec4s posB;
    vec4s colorA;
    vec4s colorB;
    vec4s uv;
    vec4s axisA; // Shared tangent at point A
    vec4s axisB; // Shared tangent at point B

    uint32_t textureId;
    uint32_t flags;
    float panSpeed;
} RibbonSegSSBO;
typedef struct
{
    u32 count;
    RibbonSegSSBO instances[MAX_QUAD_INSTANCES];
} RibbonQueue;

extern RibbonQueue ribbonQueues[PIPE_RIBBON_COUNT];
static inline RibbonSegSSBO *Sol_Render_GetNext_RibbonSeg(PipelineId kind)
{
    u32 idx = kind - PIPE_RIBBON_START;
    assert(idx < PIPE_RIBBON_COUNT && "Ribbon idx OOB");
    RibbonQueue *q      = &ribbonQueues[idx];
    RibbonSegSSBO *ssbo = &q->instances[q->count++];
    *ssbo               = (RibbonSegSSBO){0};
    return ssbo;
}

typedef enum
{
    QUADTYPE_FACECAM,
    QUADTYPE_QUAT,
} QuadType;
typedef struct QuadSSBO
{
    vec4s pos;     // 16 w=scale
    vec4s rect;    // 32 x,y=offset, z,w=dims
    vec4s color;   // 48
    vec4s uv;      // 64
    vec4s rot;     // 80
    vec4s extra;   // 96
    u32 type;      // 100 0=FaceCam, 1=rot
    u32 textureId; // 104
    u32 flags;     // 108
    u32 _pad;      // 112
} QuadSSBO;
typedef struct
{
    u32 count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} QuadQueue;

extern QuadQueue quadQueues[PIPE_QUAD_COUNT];
static inline QuadSSBO *Sol_Render_GetNextQuad(PipelineId kind)
{
    u32 idx = kind - PIPE_QUAD_START;
    assert(idx < PIPE_QUAD_COUNT && "QuadKind OOB");
    QuadQueue *q   = &quadQueues[idx];
    QuadSSBO *ssbo = &q->instances[q->count++];
    *ssbo          = (QuadSSBO){0};
    return ssbo;
}

int Sol_Render_Init();
int Sol_Render_GPU_Init(void *hwnd, void *hInstance);

void Sol_Begin_Draw();
void Sol_End_Draw();

void Sol_Render_Resize(uint32_t width, uint32_t height);
void Sol_Render_Flush3D(void);
void Sol_Render_Flush2D(void);
void Sol_Render_CheckGpuUploads();

float Sol_Render_GetAspect(void);
void Sol_Render_DrawSkybox(void);
void Sol_Render_DrawLines(const SolLine *lines, int count, size_t stride);
void Sol_Render_UploadImage(u32 width, u32 height, const void *pixels, u32 id, u8 unorm);
void Sol_Render_UploadModel(SolModelData *model, u32 kind);
void Sol_Render_DrawText2D(const char *str, SolFontDesc desc);
void Sol_Render_DrawText3D(const char *str, Text3DDesc desc);