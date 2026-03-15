#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
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
    float position[3];
    float target[3];
    float fov;
    float nearClip;
    float farClip;
} SolCamera;

void Sol_Init_Vulkan(HWND hwnd, HINSTANCE hInstance);
void Sol_Begin_Draw();
void Sol_End_Draw();
void Sol_LoadGpuModel();
void Sol_DrawModel(SolGpuModel *model);
SolGpuModel Sol_UploadModel(SolModel *model);

void Sol_Init_Triangle();
void Sol_DrawTriangle(SolCamera *cam, float rotation);