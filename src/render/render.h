#pragma once
#include "sol/types.h"

#define RENDER_CLEAR_COLOR {0.0f, 0.0f, 0.0f, 1.0f}

#define MAX_MODEL_INSTANCES (1 << 14)
#define MAX_SPHERE_INSTANCES (1 << 22)
#define MAX_LINE_VERTICES 0xffffff

typedef struct SolModel     SolModel;

typedef enum
{
    DESC_ORTHO_UBO,
    DESC_SCENE_UBO,
    DESC_MODEL_SSBO,
    DESC_SKINNING_SSBO,
    DESC_FONT_ATLAS,
    DESC_PARTICLE_SSBO,
    DESC_SPHERE_SSBO,
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
    PIPE_RECT,
    PIPE_BILLBOARD,
    PIPE_LINE,
    PIPE_SPHERE,
    PIPE_COUNT,
} PipelineId;

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
    vec4 pos;
    vec4 color;
    vec4 params;
    u64  fragtype;
} BillboardSSBO;

typedef struct
{
    vec4s pos; // xyz w=radius
    vec4s color;
} SphereSSBO;

typedef struct
{
    vec3s vertex;
} VertexSSBO;

typedef struct
{
    u32 flags;
} FlagsSSBO;

// Submission Que


typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_SPHERE_INSTANCES];
} SphereSubmission;

typedef struct SolRenderState
{
    float aspect_ratio;

} SolRenderState;

extern SolRenderState sol_render_state;

int Sol_Render_Init(void *hwnd, void *hInstance);
int Sol_Render_Resource_Init();

void  Render_Camera_Update(SolCamera *cam);
void  Sol_Render_Resize(uint32_t width, uint32_t height);
void  Remake_Swapchain(uint32_t width, uint32_t height);
void *Sol_GetDescriptorMapping(DescriptorId id);

int Sol_UploadModel(SolModel *model, SolModelId id);
int Sol_UploadImage(SolImage *image, SolImageId id);

// Single Draw rendering
void Render_Draw_Line(SolLine *lines, int count);
void Render_Draw_Rectangle(vec4s rect, vec4s color, float thickness);

void Sol_Sphere_Buffer(vec4s pos, vec4s color);

// Instance rendering
void Sol_Render_DrawSphere(vec4s pos, vec4s color);

void Flush_Models(void);
void Flush_Models_Skinned(void);
void Flush_Spheres(void);
void Flush_Queue(void);

void Render_Model(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
void Render_Model_Skinned(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
// void Sol_Render_Push_Sphere(SolSphere sphere);
// void Sol_Render_Push_Line(SolLine line);
