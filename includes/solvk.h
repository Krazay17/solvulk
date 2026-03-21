#pragma once
#include <vulkan/vulkan.h>
#include "model.h"

typedef struct {
    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer       indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t       indexCount;
} SolGpuMesh;

typedef struct SolGpuModel {
    SolGpuMesh *meshes;
    uint32_t    meshCount;
} SolGpuModel;
