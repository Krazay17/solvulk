#pragma once
#include "render/render.h"
#include "sol/types.h"
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16

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

// ─── GPU data ──────────────────────────────────────────────
typedef struct
{
    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer       indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t       indexCount;
    SolMaterial    material;
} SolGpuMesh;

typedef struct
{
    SolGpuMesh *meshes;
    u32         mesh_count;
} SolGpuModel;

typedef struct SolLineVertex
{
    vec3s pos;
    vec4s color;
} SolLineVertex;

typedef struct
{
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkSampler      sampler;
} SolGpuImage;

// ─── Reusable resource types ─────────────────────────────────────
typedef struct
{
    VkBuffer       buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory memory[MAX_FRAMES_IN_FLIGHT];
    void          *mapped[MAX_FRAMES_IN_FLIGHT];
} SolFrameBuffer;

typedef struct SolDescriptor
{
    DescriptorKind        kind;
    VkDescriptorSetLayout layout;
    VkDescriptorPool      pool;
    VkDescriptorSet       sets[MAX_FRAMES_IN_FLIGHT];

    // Buffer-backed (UBO/SSBO)
    VkBuffer       buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory memory[MAX_FRAMES_IN_FLIGHT];
    void          *mapped[MAX_FRAMES_IN_FLIGHT];

    // Image-backed (texture)
    SolGpuImage image;
} SolDescriptor;

typedef struct SolDescriptorConfig
{
    VkDeviceSize       size;
    VkDescriptorType   type;
    VkShaderStageFlags stageFlags;
    DescriptorKind     kind;
    SolImageId         imageId;
} SolDescriptorConfig;

typedef struct SolPipe
{
    VkPipeline         pipeline;
    VkPipelineLayout   layout;
    uint32_t           pushSize;
    VkShaderStageFlags pushStages;
} SolPipe;

// ─── Pipeline build configs ──────────────────────────────────────
typedef enum
{
    VERTEX_SINGLE,
    VERTEX_LINE,
    VERTEX_TRI,
    VERTEX_SKINNED,
} VertexType;
typedef struct
{
    const char         *vertResource;
    const char         *fragResource;
    VertexType          type;
    int                 depthTest;
    int                 alphaBlend;
    VkCullModeFlags     cullMode;
    uint32_t            pushRangeSize;
    VkShaderStageFlags  pushStageFlags;
    VkPrimitiveTopology primitiveTopology;

    DescriptorId descId[DESC_COUNT];
    u32          descCount;
} SolPipelineConfig;

// ─── Vulkan plumbing (exists once) ───────────────────────────────

typedef struct SolVkState
{
    VkInstance       instance;
    VkDevice         device;
    VkPhysicalDevice physicalDevice;
    VkQueue          graphicsQueue;
    uint32_t         graphicsQueueFamily;
    VkCommandPool    commandPool;

    VkSurfaceKHR   surface;
    VkSwapchainKHR swapchain;
    VkFormat       swapchainImageFormat;
    VkExtent2D     swapchainExtent;
    VkImage        swapchainImages[8];
    VkImageView    swapchainImageViews[8];
    uint32_t       swapchainImageCount;

    VkImage        depthImage;
    VkDeviceMemory depthMemory;
    VkImageView    depthImageView;

    uint32_t        currentFrame;
    uint32_t        currentImageIndex;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore     imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore     renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence         inFlightFences[MAX_FRAMES_IN_FLIGHT];
} SolVkState;

// ─── Vulkan init functions ───────────────────────────────────────

VkResult SolVkInstance(SolVkState *vk);
int      SolVkSurface(SolVkState *vk, HWND hwnd, HINSTANCE hInstance);
int      SolVkPhysicalDevice(SolVkState *vk);
int      SolVkDevice(SolVkState *vk);
int      SolVkSwapchain(SolVkState *vk);
int      SolVkImageViews(SolVkState *vk);
int      SolVkDepthResources(SolVkState *vk);
int      SolVkCommandPool(SolVkState *vk);
int      SolVkSyncObjects(SolVkState *vk);

// ─── Resource creation helpers ───────────────────────────────────

int SolFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties,
                      uint32_t *outIndex);

int SolCreateBuffer(SolVkState *vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer *outBuffer, VkDeviceMemory *outMemory);

int Sol_CreateDescriptorImage(SolVkState *vk, VkImageView imageView, VkSampler sampler, VkShaderStageFlags stageFlags,
                              SolDescriptor *out);

int Sol_Pipeline_Build(SolVkState *vkstate, SolPipelineConfig *config, SolPipe *pipe);
int Sol_Descriptor_Build(SolVkState *vkstate, SolDescriptorConfig *config, SolDescriptor *out);

// ─── Render API (internal) ───────────────────────────────────────
VkCommandBuffer Command_Buffer_Get(void);
void            Bind_Pipeline(VkCommandBuffer cmd, PipelineId id);
void     Vk_SetOrtho(uint32_t width, uint32_t height);

void Render_Model(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
void Render_Model_Skinned(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);

void Flush_Models(void);
void Flush_Models_Skinned(void);
void Flush_Spheres(void);
void Flush_Queue(void);
