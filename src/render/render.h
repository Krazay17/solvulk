#pragma once
#include "sol/types.h"

#define RENDER_CLEAR_COLOR {0.0f, 0.0f, 0.0f, 1.0f}

#define MAX_MODEL_INSTANCES (1 << 14)
#define MAX_BILLBOARD_INSTANCES (1 << 20)
#define MAX_SPHERE_INSTANCES (1 << 22)
#define MAX_LINE_VERTICES 0xffffff

typedef struct SolModel SolModel;

typedef enum
{
    DESC_ORTHO_UBO,
    DESC_SCENE_UBO,
    DESC_MODEL_SSBO,
    DESC_SKINNING_SSBO,
    DESC_FONT_ATLAS,
    DESC_BILLBOARD_SSBO,
    DESC_FLAGS_SSBO,
    DESC_COUNT,
} DescriptorId;

typedef enum
{
    DESC_KIND_UBO,
    DESC_KIND_SSBO,
    DESC_KIND_IMAGE,
} DescriptorKind;

typedef enum PipelineId
{
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,
    PIPE_TEXT,
    PIPE_LINE,
    PIPE_RECT,
    PIPE_BILLBOARD,
    PIPE_COUNT,
} PipelineId;

typedef enum
{
    BILLBOARD_SPHERE,
    BILLBOARD_RECT,
    BILLBOARD_SPRITE,
    BILLBOARD_COUNT,
} BillboardKind;

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

typedef struct {
    vec4 pos;        // xyz = world position, w = size (uniform scale)
    vec4 color;      // base tint
    vec4 params;     // type-specific (fill amount, glow strength, icon index, …)
    u32 type;       // BILLBOARD_SPHERE, BILLBOARD_HEALTHBAR, …
    u32 _padding[3]; // pad to 16 bytes for std430 alignment
} BillboardSSBO;

typedef struct
{
    u32 flags;
} FlagsSSBO;

int Sol_Render_Init(void *hwnd, void *hInstance);
int Sol_Render_BuildPipes();

float Sol_Render_GetAspect(void);

void  Sol_Render_SetOrtho(uint32_t width, uint32_t height);
void  Sol_Render_Camera_Update(SolCamera *cam);
void  Sol_Render_Resize(uint32_t width, uint32_t height);
void  Remake_Swapchain(uint32_t width, uint32_t height);
void *Sol_GetDescriptorMapping(DescriptorId id);

int Sol_UploadImage(SolImage *image, SolImageId id);
int Sol_UploadModel(SolModel *model, SolModelId id);

// Single Draw rendering
void Render_Draw_Line(SolLine *lines, int count);
void Render_Draw_Rectangle(vec4s rect, vec4s color, float thickness);
void Sol_Render_Draw_Text(SolFontDesc desc);

// Instance rendering
void Sol_Render_Push_Sphere(vec4s pos, vec4s color);
void Sol_Render_Push_Model(SolModelId handle, ModelSSBO *inst, FlagsSSBO *flags);
void Sol_Render_Push_Model_Skinned(SolModelId handle, ModelSSBO *inst, FlagsSSBO *flags, BonesSSBO *bones);

void Flush_Models(void);
void Flush_Models_Skinned(void);
void Flush_Spheres(void);
void Flush_Queue(void);

void Render_Model(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
void Render_Model_Skinned(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
// void Sol_Render_Push_Sphere(SolSphere sphere);
// void Sol_Render_Push_Line(SolLine line);
