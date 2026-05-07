#pragma once
#include "sol/types.h"

#define RENDER_CLEAR_COLOR {0.0f, 0.0f, 0.0f, 1.0f}

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
    vec4 position;
    vec4 scale;
    vec4 rotation;
    vec4 color;
    vec4 material;
} ModelSSBO;

typedef struct
{
    mat4 bones[MAX_BONES];
} SkinningSSBO;

typedef struct
{
    vec4 rec;
    vec4 c;
    vec4 extras;
} ShaderPushRect;

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
    ModelSSBO  instances[MAX_MODEL_INSTANCES];
    FlagsSSBO  flags[MAX_MODEL_INSTANCES];
    SolModelId handles[MAX_MODEL_INSTANCES];
} ModelSubmission;

typedef struct
{
    u32          count;
    ModelSSBO    modelSSBO[MAX_MODEL_INSTANCES];
    FlagsSSBO    flags[MAX_MODEL_INSTANCES];
    SkinningSSBO instances[MAX_MODEL_INSTANCES];
    SolModelId   handles[MAX_MODEL_INSTANCES];
} ModelSkinnedSubmission;

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

int  Render_Init(void *hwnd, void *hInstance);
int  Render_Init_Resources();
void Render_Camera_Update(SolCamera *cam);
void Render_Push_Model(SolModelDraw model);
void Render_Push_Model_Skinned(SolModelDraw model);
void Render_Push_Sphere(SolSphere sphere);
void Render_Push_Line(SolLine line);

void Render_Draw_Model(SolModelDraw draw);
void Render_Draw_Model_Skinned();
void Render_Draw_Sphere();
void Render_Draw_Line(SolLine *lines, int count);
void Render_Draw_Rectangle(vec4s rect, vec4s color, float thickness);

void Render_3d();
void Render_2d();

void  Sol_Render_Resize(uint32_t width, uint32_t height);
void  Remake_Swapchain(uint32_t width, uint32_t height);
void *Sol_GetDescriptorMapping(DescriptorId id);

void Sol_UploadModel(SolModel *model, SolModelId id);
int  Sol_UploadImage(SolImage *image, SolImageId id);

SolCamera *Sol_GetCamera();
void       Sol_Draw_Sphere(vec4s pos, vec4s color);
