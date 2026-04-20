#pragma once

#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_LINE_VERTICES 0xffff

// ─── Reusable resource types ─────────────────────────────────────

typedef struct
{
    VkBuffer buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory memory[MAX_FRAMES_IN_FLIGHT];
    void *mapped[MAX_FRAMES_IN_FLIGHT];
} SolFrameBuffer;

typedef struct
{
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
    VkBuffer buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory memory[MAX_FRAMES_IN_FLIGHT];
    void *mapped[MAX_FRAMES_IN_FLIGHT];
} SolDescriptorBuffer;

typedef struct
{
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet set;
} SolDescriptorImage;

typedef struct
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
} SolGpuImage;

// ─── GPU data ──────────────────────────────────────────────

typedef struct
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t indexCount;
    SolMaterial material;
} SolGpuMesh;

typedef struct
{
    SolGpuMesh *meshes;
    u32 mesh_count;
} SolGpuModel;

typedef struct SolLineVertex
{
    vec3s pos, color;
}SolLineVertex;

// ─── Shader data (matches GLSL layouts) ──────────────────────────

typedef struct
{
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
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
    mat4 ortho;
    float x, y, w, h;
    float u, v, uw, vh;
    float r, g, b, a;
} SolTextPush;

// ─── Font data ───────────────────────────────────────────────────

typedef struct
{
    float u, v, uw, vh;
    float xoffset;
    float ytop;
    float yoffset;
    float yadvance;
} SolGlyph;

typedef struct
{
    float l, b, r, t;
} TextBounds;

// ─── Pipeline + resource groupings ───────────────────────────────

typedef struct
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
} SolPipeline;

typedef struct SolPipeModel
{
    SolPipeline pipe;
    SolDescriptorBuffer modelSSBO;
} SolPipeModel;

typedef struct SolPipeRay
{
    SolPipeline pipe;
    SolDescriptorBuffer sceneUBO;
}SolPipeRay;

typedef struct
{
    SolPipeline pipe;
} SolPipeRect;

typedef struct
{
    SolPipeline pipe;
    SolGpuImage fontAtlas;
    SolDescriptorImage fontDesc;
    SolGlyph glyphs[128];
} SolPipeText;

// ─── Pipeline build configs ──────────────────────────────────────
typedef enum
{
    VERTEX_TRI,
    VERTEX_LINE,
} VertexType;
typedef struct
{
    const char *vertResource;
    const char *fragResource;
    VertexType type;
    int depthTest;
    int alphaBlend;
    VkCullModeFlags cullMode;
    uint32_t pushRangeSize;
    VkShaderStageFlags pushStageFlags;
    VkPrimitiveTopology primitiveTopology;
} SolPipelineConfig;

// ─── Vulkan plumbing (exists once) ───────────────────────────────

typedef struct SolVkState
{
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;
    VkCommandPool commandPool;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    VkImage swapchainImages[8];
    VkImageView swapchainImageViews[8];
    uint32_t swapchainImageCount;

    VkImage depthImage;
    VkDeviceMemory depthMemory;
    VkImageView depthImageView;

    uint32_t currentFrame;
    uint32_t currentImageIndex;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
} SolVkState;

// ─── Vulkan init functions ───────────────────────────────────────

VkResult SolVkInstance(SolVkState *vk);
int SolVkSurface(SolVkState *vk, HWND hwnd, HINSTANCE hInstance);
int SolVkPhysicalDevice(SolVkState *vk);
int SolVkDevice(SolVkState *vk);
int SolVkSwapchain(SolVkState *vk);
int SolVkImageViews(SolVkState *vk);
int SolVkDepthResources(SolVkState *vk);
int SolVkCommandPool(SolVkState *vk);
int SolVkSyncObjects(SolVkState *vk);

// ─── Resource creation helpers ───────────────────────────────────

int SolFindMemoryType(VkPhysicalDevice physicalDevice,
                      uint32_t typeFilter,
                      VkMemoryPropertyFlags properties,
                      uint32_t *outIndex);

int SolCreateBuffer(SolVkState *vk,
                    VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer *outBuffer,
                    VkDeviceMemory *outMemory);

int Sol_CreateDescriptorBuffer(SolVkState *vk,
                               VkDeviceSize size,
                               VkDescriptorType type,
                               VkShaderStageFlags stageFlags,
                               SolDescriptorBuffer *out);

int Sol_CreateDescriptorImage(SolVkState *vk,
                              VkImageView imageView,
                              VkSampler sampler,
                              VkShaderStageFlags stageFlags,
                              SolDescriptorImage *out);

int Sol_UploadImage(SolVkState *vkstate,
                    const void *pixels,
                    uint32_t width,
                    uint32_t height,
                    VkFormat format,
                    SolGpuImage *out);

int Sol_BuildPipeline(SolVkState *vk,
                      SolPipelineConfig *config,
                      VkDescriptorSetLayout *descLayouts,
                      uint32_t descLayoutCount,
                      SolPipeline *out);

// ─── Render API (internal) ───────────────────────────────────────

VkCommandBuffer Command_Buffer_Get(void);
void Bind_Pipeline(VkCommandBuffer cmd, VkPipeline pipeline);
void Sol_UploadModel(SolModel *model, SolModelId modelId);
void *Sol_ModelBuffer_Get(void);
void Sol_ParseFontMetrics(const char *json, float atlasW, float atlasH, SolGlyph *glyphs);
TextBounds ParseBounds(const char *p, const char *end);
int Sol_Pipeline_BuildAllDefault(SolVkState *vkstate);