#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <solmath.h>
#include "soldef.h"
#include "model.h"

static inline float SolColorF(uint8_t c) { return c / 255.0f; }

typedef struct {
    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer       indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t       indexCount;
} SolGpuMesh;

typedef struct {
    SolGpuMesh *meshes;
    uint32_t    meshCount;
} SolGpuModel;

typedef struct {
    vec3 position;
    vec3 target;
    float fov;
    float nearClip;
    float farClip;
    mat4 proj;
    mat4 view;
} SolCamera;

typedef struct {
    const char *vertResource;
    const char *fragResource;
    // rasterizer
    int depthTest;
    int alphaBlend;
    int cullBackface;

    // push constants
    uint32_t pushRangeSize;

    // descriptors (NULL if none)
    VkDescriptorSetLayout *descLayouts;
    uint32_t descLayoutCount;
} SolPipelineConfig;

typedef struct {
    mat4 ortho;
    float x, y, w, h;      // quad position in screen space
    float u, v, uw, vh;    // UV rect in atlas
    float r, g, b, a;      // color
} SolTextPush;

typedef enum
{
    PIPE_3D_MESH,
    PIPE_2D_BUTTON,
    PIPE_2D_TEXT,
    PIPE_COUNT
} SolPipelines;

int Sol_Init_Vulkan(HWND hwnd, HINSTANCE hInstance);
void Sol_Begin_Draw();
void Sol_End_Draw();
void Sol_Render_Resize();
SolGpuModel Sol_UploadModel(SolModel *model);

void Sol_Camera_Update(vec3 pos, vec3 target);
void Sol_DrawModel(SolGpuModel *model, vec3 pos, float rotY);
void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);

void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color);
float Sol_MeasureText(const char *str, float size);