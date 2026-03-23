#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include <cglm/cglm.h>
#include "vk_init_types.h"

#include "sol_core.h"
#include "render.h"

#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_GPU_MODELS 256

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

SolCamera renderCam = {
    .fov = 60.0f,
    .nearClip = 0.1f,
    .farClip = 5000.0f,
};
static mat4 ortho2d;
static SolGlyph glyphs[128] = {0};
static SolGpuModel gpuModels[MAX_GPU_MODELS] = {0};
static SolModelHandle modelCount = 0;

static VkPipeline graphicsPipeline[PIPE_COUNT] = {VK_NULL_HANDLE};
static VkPipelineLayout pipelineLayout[PIPE_COUNT] = {VK_NULL_HANDLE};

static SolPipelineConfig pipes[PIPE_COUNT] =
    {
        {
            .vertResource = "ID_SHADER_BASIC3D",
            .fragResource = "ID_SHADER_BASIC3DFRAG",
            .depthTest = 1,
            .alphaBlend = 1,
            .cullBackface = 1,
            .pushRangeSize = sizeof(float) * 32,
            .descLayoutCount = 0,
            .descLayouts = NULL,
        },
        {
            .vertResource = "ID_SHADER_BASICRECT",
            .fragResource = "ID_SHADER_BASICRECTFRAG",
            .depthTest = 0,
            .alphaBlend = 1,
            .cullBackface = 0,
            .pushRangeSize = sizeof(float) * 32,
            .descLayoutCount = 0,
            .descLayouts = NULL,
        },
        {
            .vertResource = "ID_SHADER_BASICTEXT",
            .fragResource = "ID_SHADER_BASICTEXTFRAG",
            .depthTest = 0,
            .alphaBlend = 0,
            .cullBackface = 0,
            .pushRangeSize = sizeof(float) * 32,
            .descLayoutCount = 1,
            .descLayouts = NULL,
        },
};

static uint32_t currentFrame = 0;
static uint32_t currentImageIndex = 0;
static uint32_t currentBoundPipeline = -1;

static VkInstance instance = VK_NULL_HANDLE;
static VkSurfaceKHR surface = VK_NULL_HANDLE;
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static uint32_t graphicsQueueFamily = 0;
static VkDevice device = VK_NULL_HANDLE;
static VkQueue graphicsQueue = VK_NULL_HANDLE;
static VkSwapchainKHR swapchain = VK_NULL_HANDLE;
static VkFormat swapchainImageFormat;
static VkExtent2D swapchainExtent;
static VkImage swapchainImages[8];
static uint32_t swapchainImageCount;
static VkImageView swapchainImageViews[8];
static VkCommandPool commandPool;
static VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
static VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
static VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
static VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
static VkImage depthImage = VK_NULL_HANDLE;
static VkDeviceMemory depthMemory = VK_NULL_HANDLE;
static VkImageView depthImageView = VK_NULL_HANDLE;

// font
static VkImage fontImage = VK_NULL_HANDLE;
static VkDeviceMemory fontMemory = VK_NULL_HANDLE;
static VkImageView fontImageView = VK_NULL_HANDLE;
static VkSampler fontSampler = VK_NULL_HANDLE;
static VkDescriptorSetLayout fontDescLayout = VK_NULL_HANDLE;
static VkDescriptorPool fontDescPool = VK_NULL_HANDLE;
static VkDescriptorSet fontDescSet = VK_NULL_HANDLE;

// forward declare static functions at the top
static VkResult SolVkInstance();
static int SolVkSurface(HWND hwnd, HINSTANCE hInstance);
static int SolVkPhysicalDevice();
static int SolVkDevice();
static int SolVkSwapchain();
static int SolVkImageViews();
static int SolVkFontTexture();
static int SolVkFontDescriptors();
static int SolVkPipeline(SolPipelineConfig pipeConfig, VkPipeline *outPipeline, VkPipelineLayout *outLayout);
static int SolVkCommandPool();
static int SolVkSyncObjects();
static int SolVkDepthResources();
static VkCommandBuffer SolGetCmd();
static uint32_t SolFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
static int SolCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           VkBuffer *outBuffer, VkDeviceMemory *outMemory);
static void SetOrtho();
static void Sol_ParseFontMetrics(const char *json, float atlasW, float atlasH);

static float SolColorF(uint8_t c) { return c / 255.0f; }

int Sol_Init_Vulkan(void *hwnd, void *hInstance)
{
    SolResource metrics = Sol_LoadResource("ID_FONT_METRICS");
    if (metrics.data)
    {
        printf("JSON preview:\n%.500s\n", (const char *)metrics.data);
        Sol_ParseFontMetrics((const char *)metrics.data, 224.0f, 224.0f);
    }

    if (SolVkInstance() != 0)
        return 1;
    if (SolVkSurface(hwnd, hInstance) != 0)
        return 2;
    if (SolVkPhysicalDevice() != 0)
        return 3;
    if (SolVkDevice() != 0)
        return 4;
    if (SolVkSwapchain() != 0)
        return 5;
    if (SolVkImageViews() != 0)
        return 6;
    if (SolVkDepthResources() != 0)
        return 7;
    if (SolVkCommandPool() != 0)
        return 8;

    if (SolVkFontTexture() != 0)
        return 9;
    if (SolVkFontDescriptors() != 0)
        return 10;

    pipes[PIPE_2D_TEXT].descLayouts = &fontDescLayout;

    for (int i = 0; i < PIPE_COUNT; ++i)
    {
        if (SolVkPipeline(
                pipes[i],
                &graphicsPipeline[i],
                &pipelineLayout[i]) != 0)
            return 11;
    }

    if (SolVkSyncObjects() != 0)
        return 12;

    SetOrtho();
    return 0;
}

static Bounds ParseBounds(const char *p, const char *end)
{
    Bounds bounds = {0};
    const char *open = strchr(p, '{');
    const char *close = open ? strchr(open, '}') : NULL;
    if (!open || !close || open > end)
        return bounds;

    const char *kl = strstr(open, "\"left\":");
    const char *kb = strstr(open, "\"bottom\":");
    const char *kr = strstr(open, "\"right\":");
    const char *kt = strstr(open, "\"top\":");

    if (kl && kl < close)
        bounds.l = strtof(kl + 7, NULL);
    if (kb && kb < close)
        bounds.b = strtof(kb + 9, NULL);
    if (kr && kr < close)
        bounds.r = strtof(kr + 8, NULL);
    if (kt && kt < close)
        bounds.t = strtof(kt + 6, NULL);
    return bounds;
}

static void Sol_ParseFontMetrics(const char *json, float atlasW, float atlasH)
{
    const char *p = strstr(json, "\"glyphs\":[");
    if (!p)
        return;
    p += 10;

    while (*p)
    {
        const char *obj = strchr(p, '{');
        if (!obj)
            break;
        p = obj + 1;

        // find end of glyph block
        const char *end = p;
        int depth = 1;
        while (*end && depth > 0)
        {
            if (*end == '{')
                depth++;
            if (*end == '}')
                depth--;
            end++;
        }

        const char *uni = strstr(p, "\"unicode\":");
        if (!uni || uni > end)
        {
            p = end;
            continue;
        }
        int charId = (int)strtol(uni + 10, NULL, 10);
        if (charId < 0 || charId >= 128)
        {
            p = end;
            continue;
        }

        const char *adv = strstr(p, "\"advance\":");
        const char *plane = strstr(p, "\"planeBounds\":");
        const char *atlas = strstr(p, "\"atlasBounds\":");

        Bounds pl = plane && plane < end ? ParseBounds(plane, end) : (Bounds){0};
        Bounds ab = atlas && atlas < end ? ParseBounds(atlas, end) : (Bounds){0};

        glyphs[charId].u = ab.l / atlasW;
        glyphs[charId].v = ab.b / atlasH;
        glyphs[charId].uw = (ab.r - ab.l) / atlasW;
        glyphs[charId].vh = (ab.t - ab.b) / atlasH;
        glyphs[charId].xoffset = pl.l;
        glyphs[charId].ytop = pl.t;
        glyphs[charId].yoffset = pl.b;
        glyphs[charId].yadvance = adv ? strtof(adv + 10, NULL) : 0.0f;

        p = end;
    }
}

static void SetOrtho()
{
    glm_ortho(0.0f, swapchainExtent.width, 0.0f, swapchainExtent.height, -1.0f, 1.0f, ortho2d);
}

static VkResult SolVkInstance()
{
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Sol Vulk";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to create Vulkan instance", "Error", MB_OK);
    }
    return 0;
}

static int SolVkSurface(HWND hwnd, HINSTANCE hInstance)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {0};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = hwnd;
    surfaceCreateInfo.hinstance = hInstance;

    VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to create Vulkan surface", "Error", MB_OK);
        return 1;
    }
    return 0;
}

static int SolVkPhysicalDevice()
{
    // get count first
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        MessageBox(NULL, "No Vulkan capable GPU found", "Error", MB_OK);
        return 1;
    }
    assert(deviceCount <= MAX_DEVICE_QUERY);
    // get the actual devices
    VkPhysicalDevice devices[8]; // 8 is plenty
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // pick the first discrete GPU we find, fallback to first device
    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice = devices[i];
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE)
    {
        physicalDevice = devices[0]; // fallback
    }

    // find a queue family that supports graphics
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

    assert(queueFamilyCount <= MAX_QUEUE_FAMILIES);
    VkQueueFamilyProperties queueFamilies[16];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsQueueFamily = i;
            break;
        }
    }

    VkPhysicalDeviceProperties selectedProps;
    vkGetPhysicalDeviceProperties(physicalDevice, &selectedProps);

    printf("%s\n", selectedProps.deviceName);

    return 0;
}

static int SolVkDevice()
{
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char *deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature = {0};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.pNext = &dynamicRenderingFeature;

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to create logical device", "Error", MB_OK);
        return 1;
    }

    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    return 0;
}

static int SolVkDepthResources()
{
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    // 1. Create Image
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchainExtent.width;
    imageInfo.extent.height = swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(device, &imageInfo, NULL, &depthImage);

    // 2. Memory Requirements & Allocation
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = SolFindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(device, &allocInfo, NULL, &depthMemory);
    vkBindImageMemory(device, depthImage, depthMemory, 0);

    // 3. Create View
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &viewInfo, NULL, &depthImageView);
    return 0;
}

static int SolVkSwapchain()
{
    // --- query what the surface supports ---
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    // --- choose surface format (color format + color space) ---
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    assert(formatCount <= 16);
    VkSurfaceFormatKHR formats[16];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

    // prefer BGRA8 + sRGB, fallback to first available
    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (uint32_t i = 0; i < formatCount; i++)
    {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = formats[i];
            break;
        }
    }

    // --- choose present mode (vsync behavior) ---
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    assert(presentModeCount <= 8);
    VkPresentModeKHR presentModes[8];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

    // prefer mailbox (triple buffering), fallback to FIFO (vsync, always available)
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // --- choose extent (resolution) ---
    // use the current window size
    swapchainExtent = capabilities.currentExtent;

    // --- create the swapchain ---
    swapchainImageFormat = chosenFormat.format;

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = capabilities.minImageCount + 1; // one extra to avoid waiting
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenPresentMode;
    createInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to create swapchain", "Error", MB_OK);
        return 1;
    }

    // --- retrieve the images ---
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
    assert(swapchainImageCount <= 8);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);

    return 0;
}

static int SolVkFontTexture()
{
    SolResource res = Sol_LoadResource("ID_FONT_ATLAS");

    printf("font atlas load: data=%p size=%zu\n", res.data, res.size);
    if (!res.data)
        return 1;

    // we need to know width/height — hardcode or store in resource
    uint32_t texW = 224, texH = 224;

    // --- staging buffer ---
    VkBuffer staging;
    VkDeviceMemory stagingMem;
    SolCreateBuffer(res.size,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &staging, &stagingMem);

    void *mapped;
    vkMapMemory(device, stagingMem, 0, res.size, 0, &mapped);
    memcpy(mapped, res.data, res.size);
    vkUnmapMemory(device, stagingMem);

    // --- create image ---
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = texW;
    imageInfo.extent.height = texH;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(device, &imageInfo, NULL, &fontImage);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, fontImage, &memReqs);
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = SolFindMemoryType(memReqs.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &allocInfo, NULL, &fontMemory);
    vkBindImageMemory(device, fontImage, fontMemory, 0);

    // --- transition + copy via one-time command buffer ---
    VkCommandPool transferPool;
    VkCommandPoolCreateInfo transferPoolInfo = {0};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = graphicsQueueFamily;
    transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    vkCreateCommandPool(device, &transferPoolInfo, NULL, &transferPool);

    VkCommandBufferAllocateInfo cmdAlloc = {0};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = transferPool; // use transferPool not commandPool
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // transition to transfer dst
    VkImageMemoryBarrier toTransfer = {0};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = fontImage;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.layerCount = 1;
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &toTransfer);

    // copy buffer to image
    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = texW;
    region.imageExtent.height = texH;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(cmd, staging, fontImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // transition to shader read
    VkImageMemoryBarrier toRead = {0};
    toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.image = fontImage;
    toRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toRead.subresourceRange.levelCount = 1;
    toRead.subresourceRange.layerCount = 1;
    toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &toRead);

    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(graphicsQueue);
    vkDestroyBuffer(device, staging, NULL);
    vkFreeMemory(device, stagingMem, NULL);
    vkDestroyCommandPool(device, transferPool, NULL); // frees cmd implicitly

    // --- image view ---
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, NULL, &fontImageView);

    // --- sampler ---
    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device, &samplerInfo, NULL, &fontSampler);

    return 0;
}

static int SolVkFontDescriptors()
{
    // --- layout ---
    VkDescriptorSetLayoutBinding binding = {0};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &fontDescLayout);

    // --- pool ---
    VkDescriptorPoolSize poolSize = {0};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(device, &poolInfo, NULL, &fontDescPool);

    // --- allocate set ---
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = fontDescPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &fontDescLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &fontDescSet);

    // --- write ---
    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.sampler = fontSampler;
    imageInfo.imageView = fontImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {0};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = fontDescSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

    return 0;
}

static int SolVkImageViews()
{
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &createInfo, NULL, &swapchainImageViews[i]);
        if (result != VK_SUCCESS)
        {
            MessageBox(NULL, "Failed to create image view", "Error", MB_OK);
            return 1;
        }
    }

    return 0;
}

static int SolVkPipeline(SolPipelineConfig pipeConfig, VkPipeline *outPipeline, VkPipelineLayout *outLayout)
{
    // --- load shader bytecode ---
    SolResource vertRes = Sol_LoadResource(pipeConfig.vertResource);
    SolResource fragRes = Sol_LoadResource(pipeConfig.fragResource);

    if (!vertRes.data || !fragRes.data)
        return 1;

    // --- create shader modules ---
    VkShaderModuleCreateInfo vertModuleInfo = {0};
    vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize = vertRes.size;
    vertModuleInfo.pCode = (uint32_t *)vertRes.data;

    VkShaderModuleCreateInfo fragModuleInfo = {0};
    fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize = fragRes.size;
    fragModuleInfo.pCode = (uint32_t *)fragRes.data;

    VkShaderModule vertModule, fragModule;
    vkCreateShaderModule(device, &vertModuleInfo, NULL, &vertModule);
    vkCreateShaderModule(device, &fragModuleInfo, NULL, &fragModule);

    // --- shader stages ---
    VkPipelineShaderStageCreateInfo vertStage = {0};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {0};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // binding — one buffer, per-vertex data
    VkVertexInputBindingDescription binding = {0};
    binding.binding = 0;
    binding.stride = sizeof(SolVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // attributes — position, normal, uv
    VkVertexInputAttributeDescription attrs[3] = {0};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(SolVertex, position);

    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(SolVertex, normal);

    attrs[2].location = 2;
    attrs[2].binding = 0;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(SolVertex, uv);

    // --- vertex input ---
    VkPipelineVertexInputStateCreateInfo vertexInput = {0};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 3;
    vertexInput.pVertexAttributeDescriptions = attrs;

    // --- input assembly (what shape to draw) ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // --- viewport and scissor (dynamic so we can resize) ---
    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // --- rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = pipeConfig.cullBackface ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    // --- multisampling (disabled) ---
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // --- color blending (no blending, just write output) ---
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (pipeConfig.alphaBlend)
    {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // --- dynamic state (viewport and scissor set at draw time) ---
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPushConstantRange pushRange = {0};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = pipeConfig.pushRangeSize;

    // layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = pipeConfig.descLayoutCount;
    pipelineLayoutInfo.pSetLayouts = pipeConfig.descLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = pipeConfig.pushRangeSize > 0 ? 1 : 0;
    pipelineLayoutInfo.pPushConstantRanges = pipeConfig.pushRangeSize > 0 ? &pushRange : NULL;
    vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, outLayout);

    // --- dynamic rendering info (replaces render pass) ---
    VkPipelineRenderingCreateInfo renderingInfo = {0};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &swapchainImageFormat;
    renderingInfo.depthAttachmentFormat = pipeConfig.depthTest
                                              ? VK_FORMAT_D32_SFLOAT
                                              : VK_FORMAT_UNDEFINED;

    // ---- depth stencil ----
    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = pipeConfig.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = pipeConfig.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // --- create the pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = *outLayout;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, outPipeline);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to create graphics pipeline", "Error", MB_OK);
        return 1;
    }

    // shader modules no longer needed after pipeline creation
    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);

    return 0;
}

static int SolVkCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(device, &poolInfo, NULL, &commandPool);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to create command pool", "Error", MB_OK);
        return 1;
    }

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers);
    if (result != VK_SUCCESS)
    {
        MessageBox(NULL, "Failed to allocate command buffers", "Error", MB_OK);
        return 1;
    }

    return 0;
}

static int SolVkSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame doesn't wait forever

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS)
        {
            MessageBox(NULL, "Failed to create sync objects", "Error", MB_OK);
            return 1;
        }
    }

    return 0;
}

void Sol_Begin_Draw()
{
    currentBoundPipeline = -1;
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                          imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, &currentImageIndex);

    VkCommandBuffer currentCmd = commandBuffers[currentFrame];
    vkResetCommandBuffer(currentCmd, 0);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(currentCmd, &beginInfo);

    VkImageMemoryBarrier depthBarrier = {0};
    depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.image = depthImage;
    depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthBarrier.subresourceRange.levelCount = 1;
    depthBarrier.subresourceRange.layerCount = 1;
    depthBarrier.srcAccessMask = 0;
    depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(currentCmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0, 0, NULL, 0, NULL, 1, &depthBarrier);

    // clear depth
    VkClearValue depthClear = {0};
    depthClear.depthStencil.depth = 1.0f;

    VkRenderingAttachmentInfo depthAttachment = {0};
    depthAttachment.clearValue = depthClear;
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // transition → color attachment
    VkImageMemoryBarrier toRender = {0};
    toRender.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRender.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toRender.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toRender.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRender.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRender.image = swapchainImages[currentImageIndex];
    toRender.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toRender.subresourceRange.levelCount = 1;
    toRender.subresourceRange.layerCount = 1;
    toRender.srcAccessMask = 0;
    toRender.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(currentCmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, NULL, 0, NULL, 1, &toRender);

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfo colorAttachment = {0};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchainImageViews[currentImageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColor;

    VkRenderingInfo renderingInfo = {0};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = swapchainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(currentCmd, &renderingInfo);

    VkViewport viewport = {0};
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(currentCmd, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(currentCmd, 0, 1, &scissor);
}

void Sol_End_Draw()
{
    VkCommandBuffer currentCmd = commandBuffers[currentFrame];
    vkCmdEndRendering(currentCmd);

    // transition → present
    VkImageMemoryBarrier toPresent = {0};
    toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image = swapchainImages[currentImageIndex];
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;
    toPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstAccessMask = 0;
    vkCmdPipelineBarrier(currentCmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &toPresent);

    vkEndCommandBuffer(currentCmd);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentCmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;

    vkQueuePresentKHR(graphicsQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Sol_Render_Resize()
{
    if (swapchainExtent.width == 0 || swapchainExtent.height == 0)
        return;
    vkDeviceWaitIdle(device);

    // destroy old
    vkDestroyImageView(device, depthImageView, NULL);
    vkDestroyImage(device, depthImage, NULL);
    vkFreeMemory(device, depthMemory, NULL);

    for (uint32_t i = 0; i < swapchainImageCount; i++)
        vkDestroyImageView(device, swapchainImageViews[i], NULL);

    vkDestroySwapchainKHR(device, swapchain, NULL);

    // recreate
    SolVkSwapchain();
    SolVkImageViews();
    SolVkDepthResources();
    SetOrtho();
    solState.windowWidth = swapchainExtent.width;
    solState.windowHeight = swapchainExtent.height;
}

static uint32_t SolFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    printf("Failed to find suitable memory type\n");
    return 0;
}

static int SolCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           VkBuffer *outBuffer, VkDeviceMemory *outMemory)
{
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, NULL, outBuffer) != VK_SUCCESS)
    {
        printf("Failed to create buffer\n");
        return 1;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, *outBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = SolFindMemoryType(memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, NULL, outMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate buffer memory\n");
        return 1;
    }

    vkBindBufferMemory(device, *outBuffer, *outMemory, 0);
    return 0;
}

SolModelHandle Sol_UploadModel(SolModel *model)
{
    modelCount++;
    SolGpuModel gpuModel = {0};
    gpuModel.meshCount = model->meshCount;
    gpuModel.meshes = malloc(sizeof(SolGpuMesh) * model->meshCount);
    memset(gpuModel.meshes, 0, sizeof(SolGpuMesh) * model->meshCount);

    for (uint32_t m = 0; m < model->meshCount; m++)
    {
        SolMesh *src = &model->meshes[m];
        SolGpuMesh *dst = &gpuModel.meshes[m];

        dst->indexCount = src->indexCount;

        // --- vertex buffer ---
        VkDeviceSize vertSize = sizeof(SolVertex) * src->vertexCount;

        VkBuffer stagingVB;
        VkDeviceMemory stagingVBMem;
        SolCreateBuffer(vertSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &stagingVB, &stagingVBMem);

        void *vertData;
        vkMapMemory(device, stagingVBMem, 0, vertSize, 0, &vertData);
        memcpy(vertData, src->vertices, vertSize);
        vkUnmapMemory(device, stagingVBMem);

        SolCreateBuffer(vertSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &dst->vertexBuffer, &dst->vertexMemory);

        // --- index buffer ---
        VkDeviceSize idxSize = sizeof(uint32_t) * src->indexCount;

        VkBuffer stagingIB;
        VkDeviceMemory stagingIBMem;
        SolCreateBuffer(idxSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &stagingIB, &stagingIBMem);

        void *idxData;
        vkMapMemory(device, stagingIBMem, 0, idxSize, 0, &idxData);
        memcpy(idxData, src->indices, idxSize);
        vkUnmapMemory(device, stagingIBMem);

        SolCreateBuffer(idxSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &dst->indexBuffer, &dst->indexMemory);

        // --- copy staging → GPU via command buffer ---
        VkCommandBufferAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer copyCmd;
        vkAllocateCommandBuffers(device, &allocInfo, &copyCmd);

        VkCommandBufferBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(copyCmd, &beginInfo);

        VkBufferCopy vertCopy = {0, 0, vertSize};
        vkCmdCopyBuffer(copyCmd, stagingVB, dst->vertexBuffer, 1, &vertCopy);

        VkBufferCopy idxCopy = {0, 0, idxSize};
        vkCmdCopyBuffer(copyCmd, stagingIB, dst->indexBuffer, 1, &idxCopy);

        vkEndCommandBuffer(copyCmd);

        VkSubmitInfo submitInfo = {0};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &copyCmd;
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);
        vkDestroyBuffer(device, stagingVB, NULL);
        vkFreeMemory(device, stagingVBMem, NULL);
        vkDestroyBuffer(device, stagingIB, NULL);
        vkFreeMemory(device, stagingIBMem, NULL);
    }

    printf("Model uploaded to GPU: %d meshes\n", gpuModel.meshCount);
    gpuModels[modelCount] = gpuModel;
    return modelCount;
}

static void SolBindPipeline(VkCommandBuffer cmd, int pipeIndex)
{
    if (pipeIndex == currentBoundPipeline)
        return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline[pipeIndex]);
    currentBoundPipeline = pipeIndex;
}

void Sol_DrawModel(SolModelHandle handle, vec3 pos, float rotY)
{
    // 1. Model Matrix
    mat4 modelMat;
    glm_mat4_identity(modelMat);
    glm_translate(modelMat, pos);
    glm_rotate_y(modelMat, rotY, modelMat);

    // 2. Combine into MVP (Proj * View * Model)
    mat4 mvp;
    glm_mat4_mul(renderCam.proj, renderCam.view, mvp);
    glm_mat4_mul(mvp, modelMat, mvp);

    VkCommandBuffer cmd = commandBuffers[currentFrame];
    SolBindPipeline(cmd, PIPE_3D_MESH);
    vkCmdPushConstants(cmd, pipelineLayout[PIPE_3D_MESH], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), mvp);

    // 4. Draw each mesh
    for (uint32_t i = 0; i < gpuModels[handle].meshCount; i++)
    {
        SolGpuMesh *mesh = &gpuModels[handle].meshes[i];

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // We use indexCount here, not vertexCount
        vkCmdDrawIndexed(cmd, mesh->indexCount, 1, 0, 0, 0);
    }
}

void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness)
{
    VkCommandBuffer cmd = commandBuffers[currentFrame];
    SolBindPipeline(cmd, PIPE_2D_BUTTON);

    struct
    {
        mat4 o;
        vec4 rec;
        vec4 c;
        vec4 extras;
    } push = {
        .rec = {rect.x, rect.y, rect.w, rect.h},
        .c = {SolColorF(color.r), SolColorF(color.g), SolColorF(color.b), SolColorF(color.a)},
        .extras = {thickness, 0, 0, 0},
    };
    memcpy(push.o, ortho2d, sizeof(mat4));

    vkCmdPushConstants(cmd, pipelineLayout[PIPE_2D_BUTTON],
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void Sol_Draw_Batch_Rectangle(BatchedRects *rects)
{
    //     if (count == 0 || count > MAX_RECT_INSTANCES)
    //     return;

    // // 1. Upload all instance data in one memcpy
    // memcpy(rectInstanceMapped, instances, count * sizeof(SolRectInstance));

    // // 2. Bind pipeline + descriptor set (once)
    // VkCommandBuffer cmd = commandBuffers[currentFrame];
    // SolBindPipeline(cmd, PIPE_2D_BUTTON);
    // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                         pipelineLayout[PIPE_2D_BUTTON], 0, 1,
    //                         &rectDescSet, 0, NULL);

    // // 3. Push ortho matrix (shared across all rects)
    // vkCmdPushConstants(cmd, pipelineLayout[PIPE_2D_BUTTON],
    //                    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), ortho2d);

    // // 4. ONE draw call — 6 verts per quad, `count` instances
    // vkCmdDraw(cmd, 6, count, 0, 0);
}

static VkCommandBuffer SolGetCmd()
{
    return commandBuffers[currentFrame];
}

void Sol_Camera_Update(vec3 pos, vec3 target)
{
    // 1. Update vectors
    glm_vec3_copy(pos, renderCam.position);
    glm_vec3_copy(target, renderCam.target);

    // 2. View Matrix
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat(renderCam.position, renderCam.target, up, renderCam.view);

    // 3. Projection Matrix
    float aspect = (float)swapchainExtent.width / (float)swapchainExtent.height;
    glm_perspective(glm_rad(renderCam.fov), aspect, renderCam.nearClip, renderCam.farClip, renderCam.proj);
    renderCam.proj[1][1] *= -1;
}

void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color)
{
    // atlas layout constants — match your exported atlas
    const int COLS = 16;
    const int ROWS = 6;
    const float CELL_W = 1.0f / COLS;
    const float CELL_H = 1.0f / ROWS;
    const float CHAR_W = size * 0.6f; // aspect ratio of one glyph
    const float CHAR_H = size;

    VkCommandBuffer cmd = commandBuffers[currentFrame];
    SolBindPipeline(cmd, PIPE_2D_TEXT);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout[PIPE_2D_TEXT], 0, 1, &fontDescSet, 0, NULL);

    float cursorX = x;
    for (const char *c = str; *c; c++)
    {
        int ascii = (int)(*c) - 32;
        if (ascii < 0 || ascii >= COLS * ROWS)
        {
            cursorX += CHAR_W;
            continue;
        }

        int col = ascii % COLS;
        int row = ascii / COLS;
        float baseSize = size / 32.0f;
        SolGlyph *g = &glyphs[(int)*c];

        SolTextPush push;
        memcpy(push.ortho, ortho2d, sizeof(mat4));

        float pad = 0.2f * baseSize * (224.0f / 32.0f);
        push.x = cursorX + g->xoffset * size - pad;
        push.y = y - g->ytop * size - pad;
        push.w = g->uw * 224.0f * baseSize + pad * 2.0f;
        push.h = g->vh * 224.0f * baseSize + pad * 2.0f;
        push.r = SolColorF(color.r);
        push.g = SolColorF(color.g);
        push.b = SolColorF(color.b);
        push.a = SolColorF(color.a);
        push.u = g->u;
        push.v = 1.0f - g->v - g->vh; // flip for bottom-origin atlas
        push.uw = g->uw;
        push.vh = g->vh;
        cursorX += g->yadvance * size; // only advance once, remove the CHAR_W line

        vkCmdPushConstants(cmd, pipelineLayout[PIPE_2D_TEXT],
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SolTextPush), &push);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }
}

float Sol_MeasureText(const char *str, float size)
{
    float width = 0.0f;
    for (const char *c = str; *c; c++)
    {
        int id = (int)*c;
        if (id < 0 || id >= 128)
            continue;
        width += glyphs[id].yadvance * size;
    }
    return width;
}