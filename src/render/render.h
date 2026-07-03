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

#define MAX_MODEL_INSTANCES (1 << 14)
#define MAX_RECT_INSTANCES (1 << 14)
#define MAX_FONT_INSTANCES (1 << 16)
#define MAX_QUAD_INSTANCES (1 << 20)
#define MAX_LINE_VERTICES 0xffffff

typedef struct SolCamera SolCamera;
typedef struct SolModel  SolModel;

typedef enum
{
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,

    PIPE_TEXT,
    PIPE_TEXT_3D,
    PIPE_TEXT_3D_FRONT,
    PIPE_TEXT_2D,
    PIPE_RECT,
    PIPE_RECTI,
    PIPE_LINE,

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

typedef struct
{
    double gameTime;
} GameUtilUBO;

typedef struct
{
    mat4s ortho2d;
} OrthoUBO;

typedef struct
{
    mat4s viewProjection;
    mat4s view;
    mat4s proj;
    vec4s cameraPos;
    vec4s sun;
    float aspect;
} SceneUBO;

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
    vec4s rec;
    vec4s color;
    vec4s extras;
} ShaderPushRect;

typedef struct
{
    vec4s position;
    vec4s scale;
    vec4s rotation;
    vec4s color;
    vec4s material;
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
    vec4s pos;
    vec4s color;
    vec4s params;
    u32   type;
    u32   _padding[3];
} SphereSSBO;

// pos: x, y, zindex, scale
// dims: x, y, spin, twist
typedef struct
{
    vec4s pos, color, dims, uv;
    u32   type, flags, textureID, _pad1;
} RectSSBO;
typedef struct
{
    u32      count;
    RectSSBO instances[MAX_RECT_INSTANCES];
} RectInstance;
extern RectInstance     rectQueue;
static inline RectSSBO *Sol_Render_GetNext_Rect()
{
    assert(rectQueue.count < MAX_RECT_INSTANCES && "rectQueue Full");
    return &rectQueue.instances[rectQueue.count++];
}

typedef struct
{
    vec4s pos, color, uv;
} FontSSBO;
typedef struct
{
    u32      count;
    FontSSBO instances[MAX_FONT_INSTANCES];
} FontInstance;
extern FontInstance     font2dQueue;
static inline FontSSBO *Sol_Render_GetNext_Font()
{
    assert(font2dQueue.count < MAX_FONT_INSTANCES && "font2dQueue Full");
    return &font2dQueue.instances[font2dQueue.count++];
}

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
    SolModelKind handle;
    vec4s        position;
    vec4s        scale;
    vec4s        rotation;
    vec4s        color;
    vec4s        material;
    u32          flags;
    bool         hasAnim;
    mat4        *bones;
} ModelPushDesc;

typedef struct
{
    u32          count;
    ModelSSBO    modelSSBO[MAX_MODEL_INSTANCES];
    SolModelKind handles[MAX_MODEL_INSTANCES];
} ModelSubmission;

typedef struct
{
    u32          count;
    ModelSSBO    modelSSBO[MAX_MODEL_INSTANCES];
    BonesSSBO    bones[MAX_MODEL_INSTANCES];
    SolModelKind handles[MAX_MODEL_INSTANCES];
} ModelSkinnedSubmission;

extern ModelSubmission        modelQueue;
extern ModelSkinnedSubmission skinningQueue;
static inline void            Sol_Render_GetNext_Model(SolModelKind handle, ModelSSBO *modelSSBO, BonesSSBO *bonesSSBO)
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
    u32        count;
    SphereSSBO instances[MAX_QUAD_INSTANCES];
} FireballQueue;
extern FireballQueue      fireballQueue;
static inline SphereSSBO *Sol_Render_GetNext_Fireball()
{
    FireballQueue *q = &fireballQueue;
    if (q->count >= MAX_QUAD_INSTANCES)
        return NULL;

    return &q->instances[q->count++];
}

typedef struct
{
    vec4s posA; // .xyz = world pos, .w = half-width
    vec4s posB; // .xyz = world pos, .w = half-width
    vec4s colorA;
    vec4s colorB;
    vec4s uv;
    u32   textureId, _pad0, _pad1, _pad2;
} RibbonSegSSBO;

#define MAX_RIBBON_SEGS_TOTAL (1 << 16)

typedef struct
{
    u32           count;
    RibbonSegSSBO instances[MAX_RIBBON_SEGS_TOTAL];
} RibbonQueue;

extern RibbonQueue ribbonQueue;
extern RibbonQueue ribbonQueueAdd;
extern RibbonQueue ribbonQueueFront;

static inline RibbonSegSSBO *Sol_Render_GetNext_RibbonSeg(u8 kind)
{
    u32 totalSegCount = ribbonQueue.count + ribbonQueueAdd.count + ribbonQueueFront.count;
    switch (kind)
    {
    case 0:
        if (totalSegCount >= MAX_RIBBON_SEGS_TOTAL)
            return NULL;
        return &ribbonQueue.instances[ribbonQueue.count++];
    case 1:
        if (totalSegCount >= MAX_RIBBON_SEGS_TOTAL)
            return NULL;
        return &ribbonQueueFront.instances[ribbonQueueFront.count++];
    case 2:
        if (totalSegCount >= MAX_RIBBON_SEGS_TOTAL)
            return NULL;
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
    u32   type, flags, textureId, _pad;
} QuadSSBO;
typedef struct
{
    u32      count;
    QuadSSBO instances[MAX_QUAD_INSTANCES];
} QuadQueue;
extern QuadQueue        healthQueue;
extern QuadQueue        spriteQueue0;
extern QuadQueue        spriteQueue1;
extern QuadQueue        text3dQueue;
extern QuadQueue        text3dFrontQueue;
extern QuadQueue        spriteQueueFront;
static inline QuadSSBO *Sol_Render_GetNext_Quad(u8 kind)
{
    QuadQueue *q;
    switch (kind)
    {
    case QUADKIND_SPRITE:
        q = &spriteQueue0;
        break;
    case QUADKIND_SPRITE_ADD:
        q = &spriteQueue1;
        break;
    case QUADKIND_HEALTH:
        q = &healthQueue;
        break;
    case QUADKIND_TEXT:
        q = &text3dQueue;
        break;
    case QUADKIND_TEXT_FRONT:
        q = &text3dFrontQueue;
        break;
    case QUADKIND_SPRITE_FRONT:
        q = &spriteQueueFront;
        break;
    }

    if (q->count >= MAX_QUAD_INSTANCES)
    {
        return &q->instances[q->count - 1];
    }

    return &q->instances[q->count++];
}
SceneUBO *Sol_Render_GetNext_Scene();

void Sol_Begin_Draw();
void Sol_End_Draw();

void Sol_Render_Resize(uint32_t width, uint32_t height);
void Sol_Render_Flush3D(void);
void Sol_Render_Flush2D(void);

float Sol_Render_GetAspect(void);
void  Sol_Render_DrawSkybox(void);
void  Sol_Render_DrawLine(SolLine *lines, int count);
void  Sol_Render_DrawRectangle(vec4s rect, vec4s color, float thickness, float fill);
void  Sol_Render_DrawText(SolFontDesc desc);
void  Sol_Render_UploadImage(u32 width, u32 height, const void *pixels, u32 id);
void  Sol_Render_UploadModel(SolModel *model, u32 modelId);
void  Sol_Render_DrawText2D(SolFontDesc desc);
void  Sol_Render_DrawText3D(Text3DDesc desc);