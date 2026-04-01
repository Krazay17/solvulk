#pragma once
#include <vulkan/vulkan.h>

#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_GPU_MODELS 256

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

typedef struct
{
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

typedef enum
{
    PIPE_3D_MESH,
    PIPE_2D_BUTTON,
    PIPE_2D_TEXT,
    PIPE_COUNT
} SolPipelines;

typedef struct
{
    float l, b, r, t;
} Bounds;
typedef struct
{
    float u, v, uw, vh; // UV coords in 0-1 space
    float xoffset;
    float ytop;
    float yoffset;
    float yadvance;
} SolGlyph;
typedef struct
{
    mat4 ortho;
    float x, y, w, h;   // quad position in screen space
    float u, v, uw, vh; // UV rect in atlas
    float r, g, b, a;   // color
} SolTextPush;
