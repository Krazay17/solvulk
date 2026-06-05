#pragma once
#include "render.h"

#define RENDER_CLEAR_COLOR {0.0f, 0.0f, 0.0f, 1.0f}

typedef struct SolModel   SolModel;
typedef struct SolTexture SolTexture;

typedef enum
{
    DESC_KIND_BUFFER,
    DESC_KIND_IMAGES,
} DescriptorKind;

typedef enum
{
    DESC_GAME_UBO,
    DESC_ORTHO_UBO,
    DESC_SCENE_UBO,
    DESC_MODEL_SSBO,
    DESC_SKINNING_SSBO,
    DESC_QUAD_SSBO,
    DESC_FONT_SSBO,
    DESC_RECT_SSBO,
    DESC_SPHERE_SSBO,
    DESC_IMAGES,
    DESC_COUNT,
} DescriptorId;

typedef enum
{
    BLEND_NONE,
    BLEND_ALPHA,
    BLEND_ADDITIVE,
} BlendMode;

int Sol_Render_Init(void *hwnd, void *hInstance);
int Sol_Render_BuildPipes();

void Sol_Render_SetOrtho(uint32_t width, uint32_t height);
void Remake_Swapchain(uint32_t width, uint32_t height);

void *Sol_GetDescriptorMapping(DescriptorId id);

int Sol_UploadImage(SolTexture *image, SolTextureId id);
int Sol_UploadModel(SolModel *model, SolModelId id);

void Flush_Models(void);
void Flush_Quads(void);
void Flush_Spheres(void);
void Flush_Rects(void);
void Flush_Fonts2d(void);

void Render_Model(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
void Render_Model_Skinned(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);

ShaderPushTexts Prepare_Text(SolFontDesc desc);
