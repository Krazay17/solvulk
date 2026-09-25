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

#define MAX_MODEL_INSTANCES 0x4fff
#define MAX_RECT_INSTANCES (1 << 14)
#define MAX_FONT_INSTANCES (1 << 16)
#define MAX_QUAD_INSTANCES (1 << 20)
#define MAX_BUFFER_VERTS 0xffffff

typedef struct ScModelData ScModelData;

// 1. Define category pipeline lists
#define SOL_TEXT_PIPELINES(X)                                                                                          \
    X(PIPE_TEXT, "shaders/text2d.vert.spv", "shaders/text2d.frag.spv")                                                 \
    X(PIPE_TEXT_3D, "shaders/quad.vert.spv", "shaders/text3d.frag.spv")                                                \
    X(PIPE_TEXT_3D_FRONT, "shaders/quad.vert.spv", "shaders/text3d.frag.spv")

#define SOL_SPHERE_PIPELINES(X)                                                                                        \
    X(PIPE_SPHERE, "shaders/sphere.vert.spv", "shaders/sphere.frag.spv")                                               \
    X(PIPE_DEBUG_SPHERE, "shaders/sphere.vert.spv", "shaders/sphere_debug.frag.spv")                                   \
    X(PIPE_SPHERE_FX, "shaders/sphere.vert.spv", "shaders/sphere_fx.frag.spv")                                         \
    X(PIPE_FIREBALL, "shaders/sphere.vert.spv", "shaders/sphere_fireball.frag.spv")                                    \
    X(PIPE_PLASMA, "shaders/sphere.vert.spv", "shaders/sphere_plasma.frag.spv")                                        \
    X(PIPE_PARTICLE_DRAGON, "shaders/sphere.vert.spv", "shaders/sphere_dragon.frag.spv")                               \
    X(PIPE_FRACTAL_PYRAMID, "shaders/sphere.vert.spv", "shaders/sphere_pyramid.frag.spv")

#define SOL_QUAD_PIPELINES(X)                                                                                          \
    X(PIPE_QUAD, "shaders/quad.vert.spv", "shaders/sprite.frag.spv")                                                   \
    X(PIPE_QUAD_ADD, "shaders/quad.vert.spv", "shaders/sprite.frag.spv")                                               \
    X(PIPE_QUAD_FRONT, "shaders/quad.vert.spv", "shaders/sprite.frag.spv")                                             \
    X(PIPE_HEALTHBAR, "shaders/quad.vert.spv", "shaders/healthbar.frag.spv")

#define SOL_RIBBON_PIPELINES(X)                                                                                        \
    X(PIPE_RIBBON, "shaders/ribbon.vert.spv", "shaders/sprite.frag.spv")                                               \
    X(PIPE_RIBBON_ADD, "shaders/ribbon.vert.spv", "shaders/sprite.frag.spv")                                           \
    X(PIPE_RIBBON_FRONT, "shaders/ribbon.vert.spv", "shaders/sprite.frag.spv")

// 2. Compile-time element counter trick
#define X_COUNT(id, ...) +1
#define PIPE_TEXT_COUNT (0 SOL_TEXT_PIPELINES(X_COUNT))
#define PIPE_SPHERE_COUNT (0 SOL_SPHERE_PIPELINES(X_COUNT))
#define PIPE_QUAD_COUNT (0 SOL_QUAD_PIPELINES(X_COUNT))
#define PIPE_RIBBON_COUNT (0 SOL_RIBBON_PIPELINES(X_COUNT))

// 3. Enum expansion macro
#define X_ENUM(id, ...) id,

typedef enum
{
    PIPE_SKYBOX,
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,
    PIPE_MODEL_TRANSPARENT,
    PIPE_RECT,
    PIPE_LINE,

    SOL_TEXT_PIPELINES(X_ENUM)
    SOL_SPHERE_PIPELINES(X_ENUM) SOL_QUAD_PIPELINES(X_ENUM) SOL_RIBBON_PIPELINES(X_ENUM)

        PIPE_COUNT,

    PIPE_TEXT_START = PIPE_TEXT,
    PIPE_TEXT_END   = PIPE_TEXT_START + PIPE_TEXT_COUNT,

    PIPE_SPHERE_START = PIPE_SPHERE,
    PIPE_SPHERE_END   = PIPE_SPHERE_START + PIPE_SPHERE_COUNT,

    PIPE_QUAD_START = PIPE_QUAD,
    PIPE_QUAD_END   = PIPE_QUAD_START + PIPE_QUAD_COUNT,

    PIPE_RIBBON_START = PIPE_RIBBON,
    PIPE_RIBBON_END   = PIPE_RIBBON_START + PIPE_RIBBON_COUNT,
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
    ModelKind handles[MAX_MODEL_INSTANCES];
    ModelSSBO instances[MAX_MODEL_INSTANCES];
} ModelQueue;

extern ModelQueue modelQueues[1];

static inline ModelSSBO *Sol_Render_GetNextModel(u32 pipe, ModelKind handle)
{
    ModelQueue *q = &modelQueues[pipe];
    int count     = q->count;
    if (count >= MAX_MODEL_INSTANCES)
        return NULL;

    q->handles[count] = handle;
    ModelSSBO *buffer = &q->instances[count];
    *buffer           = (ModelSSBO){0};
    q->count++;
    return buffer;
}

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

typedef struct
{
    SphereSSBO *instances;
} SphereQueue;

extern SphereQueue sphereQueues[PIPE_SPHERE_COUNT];
static inline SphereSSBO *Sol_Render_GetNextSphere(PipelineId kind)
{
    u32 idx = kind - PIPE_SPHERE_START;
    assert(idx < PIPE_SPHERE_COUNT);
    SphereQueue *q   = &sphereQueues[idx];
    SphereSSBO *ssbo = solb_next(q->instances);
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

typedef struct
{
    RibbonSegSSBO *instances;
} RibbonQueue;

extern RibbonQueue ribbonQueues[PIPE_RIBBON_COUNT];
static inline RibbonSegSSBO *Sol_Render_GetNext_RibbonSeg(PipelineId kind)
{
    u32 idx = kind - PIPE_RIBBON_START;
    assert(idx < PIPE_RIBBON_COUNT && "Ribbon idx OOB");
    RibbonQueue *q      = &ribbonQueues[idx];
    RibbonSegSSBO *ssbo = solb_next(q->instances);
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
    QuadSSBO *instances;
} QuadQueue;

extern QuadQueue quadQueues[PIPE_QUAD_COUNT];
static inline QuadSSBO *Sol_Render_GetNextQuad(PipelineId kind)
{
    u32 idx = kind - PIPE_QUAD_START;
    assert(idx < PIPE_QUAD_COUNT && "QuadKind OOB");
    QuadQueue *q   = &quadQueues[idx];
    QuadSSBO *ssbo = solb_next(q->instances);
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
void Sol_Render_UploadModel(ScModelData *model, u32 kind);
void Sol_Render_DrawText2D(const char *str, SolFontDesc desc);
void Sol_Render_DrawText3D(const char *str, Text3DDesc desc);