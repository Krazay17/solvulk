#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include "soldef.h"
#include "model.h"

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
    int         depthTest;
    int         alphaBlend;
    int         is2D;        // skips vertex input description
} SolPipelineConfig;

typedef enum
{
    PIPE_2D_BUTTON,
    PIPE_3D_MESH,
    PIPE_COUNT
} SolPipelines;

void Sol_Init_Vulkan(HWND hwnd, HINSTANCE hInstance);
void Sol_Begin_Draw();
void Sol_End_Draw();
SolGpuModel Sol_UploadModel(SolModel *model);

void Sol_Camera_Update(vec3 pos, vec3 target);
void Sol_DrawModel(SolGpuModel *model, vec3 pos, float rotY);
void Sol_Draw_Rectangle(solrect rect);