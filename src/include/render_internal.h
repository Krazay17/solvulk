#pragma once
#include <vulkan/vulkan.h>

#include "sol/render.h"

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16
#define MAX_GPU_MODELS 256
#define MAX_MODEL_INSTANCES 500000
#define MAX_BONES 64

typedef enum
{
    PIPE_3D_MESH,
    PIPE_2D_BUTTON,
    PIPE_2D_TEXT,
    PIPE_COUNT
} SolPipelineId;

typedef struct
{
    mat4 modelMatrix;
    vec4 color;
} ModelInstanceData;

typedef struct
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t indexCount;
} SolGpuMesh;

typedef struct SolGpuModel
{
    SolGpuMesh *meshes;
    uint32_t meshCount;
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

    VkDescriptorSetLayout *descLayouts;
    uint32_t descLayoutCount;
} SolPipelineConfig;

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

struct SolVkState
{
    uint32_t currentFrame;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    VkQueue graphicsQueue;
    uint32_t currentImageIndex;
    uint32_t currentBoundPipeline;

    SolGpuModel gpuModels[MAX_GPU_MODELS];

    VkPipeline pipeline[PIPE_COUNT];
    VkPipelineLayout pipelineLayout[PIPE_COUNT];

    VkBuffer modelBuffer[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory modelMemory[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSetLayout modelDescLayout;
    VkDescriptorPool modelDescPool;
    VkDescriptorSet modelDescSet[MAX_FRAMES_IN_FLIGHT];
    void *modelDataPtr[MAX_FRAMES_IN_FLIGHT];

    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    uint32_t graphicsQueueFamily;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    VkImage swapchainImages[8];
    uint32_t swapchainImageCount;
    VkImageView swapchainImageViews[8];
    VkCommandPool commandPool;
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

    VkImage depthImage;
    VkDeviceMemory depthMemory;
    VkImageView depthImageView;

    // font
    VkImage fontImage;
    VkDeviceMemory fontMemory;
    VkImageView fontImageView;
    VkSampler fontSampler;
    VkDescriptorSetLayout fontDescLayout;
    VkDescriptorPool fontDescPool;
    VkDescriptorSet fontDescSet;
};

extern SolPipelineConfig pipelineConfigs[PIPE_COUNT];
extern SolGlyph glyphs[128];
extern float solAspectRatio;

VkCommandBuffer Sol_CommandBuffer();
VkResult SolVkInstance(SolVkState *vkstate);
int SolVkSurface(SolVkState *vkstate, HWND hwnd, HINSTANCE hInstance);
int SolVkPhysicalDevice(SolVkState *vkstate);
int SolVkDevice(SolVkState *vkstate);
int SolVkSwapchain(SolVkState *vkstate);
int SolVkImageViews(SolVkState *vkstate);
int SolVkFontTexture(SolVkState *vkstate);
int SolVkSSBO(SolVkState *vkstate);
int SolVkPipeline(SolVkState *vkstate,
                  SolPipelineConfig pipeConfig,
                  VkPipeline *outPipeline,
                  VkPipelineLayout *outLayout);
int SolVkCommandPool(SolVkState *vkstate);
int SolVkSyncObjects(SolVkState *vkstate);
int SolVkDepthResources(SolVkState *vkstate);
uint32_t SolFindMemoryType(VkPhysicalDevice physicalDevice,
                           uint32_t typeFilter,
                           VkMemoryPropertyFlags properties);
int SolCreateBuffer(SolVkState *vkstate,
                    VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer *outBuffer, VkDeviceMemory *outMemory);

int Sol_Pipeline_BuildAll(SolVkState *vkstate);

int SolVkFontDescriptors(SolVkState *vkstate);
Bounds ParseBounds(const char *p, const char *end);
void Sol_ParseFontMetrics(SolVkState *vkstate, const char *json, float atlasW, float atlasH);

void Sol_UploadModel(SolModel *model, SolModelId modelId);
void *Sol_ModelBuffer_Get();