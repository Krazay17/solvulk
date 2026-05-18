#pragma once
#include "render.h"

#define RENDER_CLEAR_COLOR {0.0f, 0.0f, 0.0f, 1.0f}

#define MAX_MODEL_INSTANCES (1 << 14)
#define MAX_BILLBOARD_INSTANCES (1 << 20)
#define MAX_SPHERE_INSTANCES (1 << 20)
#define MAX_QUAD_INSTANCES (1 << 20)
#define MAX_LINE_VERTICES 0xffffff

typedef struct SolModel   SolModel;
typedef struct SolTexture SolTexture;

typedef enum
{
    DESC_KIND_BUFFER,
    DESC_KIND_IMAGES,
} DescriptorKind;

typedef enum
{
    DESC_ORTHO_UBO,
    DESC_SCENE_UBO,
    DESC_MODEL_SSBO,
    DESC_SKINNING_SSBO,
    DESC_FLAGS_SSBO,
    DESC_BILLBOARD_SSBO,
    DESC_SPHERE,
    DESC_QUAD,
    DESC_IMAGES,
    DESC_COUNT,
} DescriptorId;

typedef enum
{
    BLEND_NONE,
    BLEND_ALPHA,
    BLEND_ADDITIVE,
} BlendMode;

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

int Sol_Render_Init(void *hwnd, void *hInstance);
int Sol_Render_BuildPipes();

void Sol_Render_SetOrtho(uint32_t width, uint32_t height);
void Remake_Swapchain(uint32_t width, uint32_t height);

void *Sol_GetDescriptorMapping(DescriptorId id);

int Sol_UploadImage(SolTexture *image, SolTextureId id);
int Sol_UploadModel(SolModel *model, SolModelId id);

void Flush_Skybox(void);
void Flush_Models(void);
void Flush_Models_Skinned(void);
void Flush_Spheres(void);
void Flush_Sprites(void);
void Flush_Billboards(void);
void Flush_Queue(void);

void Render_Model(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
void Render_Model_Skinned(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);

ShaderPushTexts Prepare_Text(SolFontDesc desc);
