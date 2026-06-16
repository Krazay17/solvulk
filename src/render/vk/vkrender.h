#pragma once
#include "sol/types.h"

#include "render/render_i.h"
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16

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

typedef struct
{
    VkDeviceSize       size;
    VkShaderStageFlags stage;
} SolFrameBufferConfig;

typedef struct
{
    VkBuffer *buffers;
    void     *mapped;
} SolFrameBufferRef;

typedef struct SolBufferDescriptor
{
    DescriptorKind        kind;
    VkDescriptorSetLayout layout;
    VkDescriptorPool      pool;

    VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
    VkBuffer        buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory  memory[MAX_FRAMES_IN_FLIGHT];
    void           *mapped[MAX_FRAMES_IN_FLIGHT];

    // Image-backed (texture)
    SolGpuImage image;
} SolBufferDescriptor;

typedef struct SolImageDescriptor
{
    VkDescriptorSetLayout layout;
    VkDescriptorPool      pool;
    VkDescriptorSet       set;
} SolImageDescriptor;

typedef struct SolDescriptorConfig
{
    DescriptorKind     kind;
    VkShaderStageFlags stageFlags;
    union {
        struct
        {
            VkDeviceSize     size;
            VkDescriptorType type;
        } buffer;
        struct
        {
            int count;
        } image;
    } as;
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
    const char *vertResource;
    const char *fragResource;
    VertexType  type;

    BlendMode           blendMode;
    int                 depthTest;
    int                 depthWrite;
    VkCullModeFlags     cullMode;
    uint32_t            pushRangeSize;
    VkShaderStageFlags  pushStageFlags;
    VkPrimitiveTopology primitiveTopology;
    VkCompareOp         depthCompareOp;

    DescriptorId descId[DESC_COUNT];
    u32          descCount;
} SolPipelineConfig;

typedef struct SolImageUploadConfig
{
    u32 Uwrap;

} SolImageUploadConfig;

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
                              SolBufferDescriptor *out);

int Sol_Pipeline_Build(SolVkState *vkstate, SolPipelineConfig *config, SolPipe *pipe);
int Sol_BufferDescriptor_Build(SolVkState *vkstate, const SolDescriptorConfig *config, SolBufferDescriptor *out);
int Sol_ImageDescriptor_Build(SolVkState *vkstate, SolGpuImage *images, SolImageDescriptor *out);

// ─── Render API (internal) ───────────────────────────────────────
VkCommandBuffer   Command_Buffer_Get(void);
SolFrameBufferRef Sol_GetFrameBuffer(FrameBufferId id);

void Bind_Pipeline(VkCommandBuffer cmd, PipelineId id);
int Sol_ImageDescriptor_BuildLayout(SolVkState *vkstate, SolImageDescriptor *out);
void Sol_ImageDescriptor_UpdateSlot(SolVkState *vkstate, SolImageDescriptor *desc,
                                    SolGpuImage *image, u32 slotIndex);