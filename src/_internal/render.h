#pragma once
#include "sol/types.h"

#include <vulkan/vulkan.h>

#define MAX_MODEL_INSTANCES (1 << 14)
#define MAX_SPHERE_INSTANCES (1 << 22)
#define MAX_BONES 128

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_LINE_VERTICES 0xffffff

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

// ------Cpu data----------

// Per-bone CPU data

// ─── Shader data (matches GLSL layouts) ──────────────────────────

typedef struct
{
    mat4 ortho2d;
} OrthoUBO;

typedef struct
{
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
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
    vec4 rec;
    vec4 c;
    vec4 extras;
} ShaderPushRect;

typedef struct
{
    mat4 bones[MAX_BONES];
} SkinningSSBO;

typedef struct
{
    float x, y, w, h;
    float u, v, uw, vh;
    float r, g, b, a;
} ShaderPushText;

typedef struct
{
    vec4s pos; // xyz w=radius
    vec4s color;
} SphereSSBO;

typedef struct
{
    vec3s vertex;
} VertexSSBO;

typedef struct
{
    u32 flags;
} FlagsSSBO;

// Submission Que

typedef struct
{
    u32        count;
    ModelSSBO  instances[MAX_MODEL_INSTANCES];
    FlagsSSBO  flags[MAX_MODEL_INSTANCES];
    SolModelId handles[MAX_MODEL_INSTANCES];
} ModelSubmission;

typedef struct
{
    u32          count;
    ModelSSBO    modelSSBO[MAX_MODEL_INSTANCES];
    SkinningSSBO instances[MAX_MODEL_INSTANCES];
    FlagsSSBO    flags[MAX_MODEL_INSTANCES];
    SolModelId   handles[MAX_MODEL_INSTANCES];
} ModelSkinnedSubmission;

typedef struct
{
    u32        count;
    SphereSSBO instances[MAX_SPHERE_INSTANCES];
} SphereSubmission;

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
    vec3s pos, color;
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

int  Sol_UploadImage(const void *pixels, u32 width, u32 height, VkFormat format, SolImageId id);
void Sol_UploadModel(SolModel *model, SolModelId id);

// ─── Render API (internal) ───────────────────────────────────────

VkCommandBuffer Command_Buffer_Get(void);
void            Bind_Pipeline(VkCommandBuffer cmd, PipelineId id);
void            Parse_Font_Metrics(const char *json, float atlasW, float atlasH, SolGlyph *glyphs);
TextBounds      ParseBounds(const char *p, const char *end);

void Render_Camera_Update(vec3 pos, vec3 target);

void Sol_Submit_Sphere(vec4s pos, vec4s color);
void Sol_Submit_Model(SolModelId handle, vec3s pos, vec3s scale, versors quat, u32 flags);
void Sol_Submit_Animated_Model(SolModelId handle, vec3s pos, vec3s scale, versors quat, int animIndex, float time, u32 flags);

void Flush_Models(void);
void Flush_Models_Skinned(void);
void Flush_Spheres(void);

void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
void Sol_Draw_Model_Skinned_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);

void Sol_Skeleton_Pose(SolSkeleton *skel, int animIndex, float time, mat4 *outSkinMatrices);

static SolPipelineConfig pipe_config[PIPE_COUNT] = {
    [PIPE_TEXT] =
        {
            .vertResource      = "ID_SHADER_TEXT_V",
            .fragResource      = "ID_SHADER_TEXT_F",
            .depthTest         = 0,
            .alphaBlend        = 0,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(ShaderPushText),
            .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .descId            = {DESC_ORTHO_UBO, DESC_FONT_ATLAS},
            .descCount         = 2,
        },
    [PIPE_MODEL] =
        {
            .vertResource      = "ID_SHADER_MODEL_V",
            .fragResource      = "ID_SHADER_MODEL_F",
            .depthTest         = 1,
            .alphaBlend        = 1,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(SolMaterial),
            .pushStageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .type              = VERTEX_TRI,
            .descId            = {DESC_SCENE_UBO, DESC_MODEL_SSBO, DESC_FLAGS_SSBO},
            .descCount         = 3,
        },
    [PIPE_RECT] =
        {
            .vertResource      = "ID_SHADER_RECT_V",
            .fragResource      = "ID_SHADER_RECT_F",
            .depthTest         = 0,
            .alphaBlend        = 1,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(ShaderPushRect),
            .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .descId            = {DESC_ORTHO_UBO},
            .descCount         = 1,
        },
    [PIPE_LINE] =
        {
            .vertResource      = "ID_SHADER_LINE_V",
            .fragResource      = "ID_SHADER_LINE_F",
            .depthTest         = 1,
            .alphaBlend        = 1,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = 0,
            .pushStageFlags    = 0,
            .type              = VERTEX_LINE,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
            .descId            = {DESC_SCENE_UBO},
            .descCount         = 1,
        },
    [PIPE_SPHERE] =
        {
            .vertResource      = "ID_SHADER_SPHERE_V",
            .fragResource      = "ID_SHADER_SPHERE_F",
            .depthTest         = 1,
            .alphaBlend        = 1,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = 0,
            .pushStageFlags    = 0,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .descId            = {DESC_SCENE_UBO, DESC_SPHERE_SSBO},
            .descCount         = 2,
        },
    [PIPE_MODEL_SKINNED] =
        {
            .vertResource      = "ID_SHADER_SKINNED_V",
            .fragResource      = "ID_SHADER_MODEL_F",
            .descId            = {DESC_SCENE_UBO, DESC_MODEL_SSBO, DESC_FLAGS_SSBO, DESC_SKINNING_SSBO},
            .descCount         = 4,
            .type              = VERTEX_SKINNED,
            .depthTest         = 1,
            .alphaBlend        = 1,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(SolMaterial),
            .pushStageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        },
};

static SolDescriptorConfig desc_config[DESC_COUNT] = {
    [DESC_ORTHO_UBO] =
        {
            .size       = sizeof(OrthoUBO),
            .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_UBO,
        },
    [DESC_SCENE_UBO] =
        {
            .size       = sizeof(SceneUBO),
            .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_UBO,
        },
    [DESC_MODEL_SSBO] =
        {
            .size       = sizeof(ModelSSBO) * MAX_MODEL_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_FONT_ATLAS] =
        {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_IMAGE,
            .imageId    = SOL_IMAGE_FONT,
        },
    [DESC_PARTICLE_SSBO] =
        {
            .size       = sizeof(VertexSSBO),
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_SPHERE_SSBO] =
        {
            .size       = sizeof(SphereSSBO) * MAX_SPHERE_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_FLAGS_SSBO] =
        {
            .size       = sizeof(FlagsSSBO) * MAX_MODEL_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_SKINNING_SSBO] =
        {
            .size       = sizeof(SkinningSSBO) * MAX_MODEL_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },

};