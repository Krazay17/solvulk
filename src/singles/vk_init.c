#include <cglm/cglm.h>
#include <cglm/struct.h>

#include "sol_core.h"
#include "render_internal.h"

SolPipelineConfig pipelineConfigs[PIPE_COUNT] = {
    [PIPE_3D_MESH] = {
        .vertResource = "ID_SHADER_BASIC3D",
        .fragResource = "ID_SHADER_BASIC3DFRAG",
        .depthTest = 1,
        .alphaBlend = 1,
        .cullBackface = 1,
        .pushRangeSize = sizeof(float) * 32,
        .descLayouts = NULL,
        .descLayoutCount = 1,
    },
    [PIPE_2D_BUTTON] = {
        .vertResource = "ID_SHADER_BASICRECT",
        .fragResource = "ID_SHADER_BASICRECTFRAG",
        .depthTest = 0,
        .alphaBlend = 1,
        .cullBackface = 0,
        .pushRangeSize = sizeof(float) * 32,
        .descLayoutCount = 0,
        .descLayouts = NULL,
    },
    [PIPE_2D_TEXT] = {
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

Bounds ParseBounds(const char *p, const char *end)
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

void Sol_ParseFontMetrics(SolVkState *vkstate, const char *json, float atlasW, float atlasH)
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

VkResult SolVkInstance(SolVkState *vkstate)
{
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "Sol Vulk";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &vkstate->instance);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create Vulkan instance", "Error");
    }
    return 0;
}

int SolVkSurface(SolVkState *vkstate, HWND hwnd, HINSTANCE hInstance)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {0};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = hwnd;
    surfaceCreateInfo.hinstance = hInstance;

    VkResult result = vkCreateWin32SurfaceKHR(vkstate->instance, &surfaceCreateInfo, NULL, &vkstate->surface);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create Vulkan surface", "Error");
        return 1;
    }
    return 0;
}

int SolVkPhysicalDevice(SolVkState *vkstate)
{
    // get count first
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkstate->instance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        Sol_MessageBox("No Vulkan capable GPU found", "Error");
        return 1;
    }
    assert(deviceCount <= MAX_DEVICE_QUERY);
    // get the actual devices
    VkPhysicalDevice devices[8]; // 8 is plenty
    vkEnumeratePhysicalDevices(vkstate->instance, &deviceCount, devices);

    // pick the first discrete GPU we find, fallback to first device
    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            vkstate->physicalDevice = devices[i];
            break;
        }
    }
    if (vkstate->physicalDevice == VK_NULL_HANDLE)
    {
        vkstate->physicalDevice = devices[0]; // fallback
    }

    // find a queue family that supports graphics
    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties queueFamilies[16];
    vkGetPhysicalDeviceQueueFamilyProperties(vkstate->physicalDevice, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            vkstate->graphicsQueueFamily = i;
            break;
        }
    }

    VkPhysicalDeviceProperties selectedProps;
    vkGetPhysicalDeviceProperties(vkstate->physicalDevice, &selectedProps);

    printf("%s\n", selectedProps.deviceName);

    return 0;
}

int SolVkDevice(SolVkState *vkstate)
{
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = vkstate->graphicsQueueFamily;
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

    VkResult result = vkCreateDevice(vkstate->physicalDevice, &createInfo, NULL, &vkstate->device);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create logical device", "Error");
        // MessageBox(NULL, "Failed to create logical device", "Error", MB_OK);
        return 1;
    }

    vkGetDeviceQueue(vkstate->device, vkstate->graphicsQueueFamily, 0, &vkstate->graphicsQueue);
    return 0;
}

int SolVkDepthResources(SolVkState *vkstate)
{
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    // 1. Create Image
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = vkstate->swapchainExtent.width;
    imageInfo.extent.height = vkstate->swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(vkstate->device, &imageInfo, NULL, &vkstate->depthImage);

    // 2. Memory Requirements & Allocation
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkstate->device, vkstate->depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = SolFindMemoryType(vkstate->physicalDevice,
                                                  memReqs.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(vkstate->device, &allocInfo, NULL, &vkstate->depthMemory);
    vkBindImageMemory(vkstate->device, vkstate->depthImage, vkstate->depthMemory, 0);

    // 3. Create View
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkstate->depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(vkstate->device, &viewInfo, NULL, &vkstate->depthImageView);
    return 0;
}

int SolVkSwapchain(SolVkState *vkstate)
{
    // --- query what the surface supports ---
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkstate->physicalDevice, vkstate->surface, &capabilities);

    // --- choose surface format (color format + color space) ---
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate->physicalDevice, vkstate->surface, &formatCount, NULL);
    assert(formatCount <= 16);
    VkSurfaceFormatKHR formats[16];
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkstate->physicalDevice, vkstate->surface, &formatCount, formats);

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
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkstate->physicalDevice, vkstate->surface, &presentModeCount, NULL);
    assert(presentModeCount <= 8);
    VkPresentModeKHR presentModes[8];
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkstate->physicalDevice, vkstate->surface, &presentModeCount, presentModes);

    // prefer mailbox (triple buffering), fallback to FIFO (vsync, always available)
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            chosenPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        }
        // Mailbox is the best of both worlds if Immediate isn't there
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    // --- choose extent (resolution) ---
    // use the current window size
    vkstate->swapchainExtent = capabilities.currentExtent;

    // --- create the swapchain ---
    vkstate->swapchainImageFormat = chosenFormat.format;

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkstate->surface;
    createInfo.minImageCount = capabilities.minImageCount + 1; // one extra to avoid waiting
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = vkstate->swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenPresentMode;
    createInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(vkstate->device, &createInfo, NULL, &vkstate->swapchain);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create swapchain", "Error");
        // MessageBox(NULL, "Failed to create swapchain", "Error", MB_OK);
        return 1;
    }

    // --- retrieve the images ---
    vkGetSwapchainImagesKHR(vkstate->device, vkstate->swapchain, &vkstate->swapchainImageCount, NULL);
    assert(vkstate->swapchainImageCount <= 8);
    vkGetSwapchainImagesKHR(vkstate->device, vkstate->swapchain, &vkstate->swapchainImageCount, vkstate->swapchainImages);

    return 0;
}

int SolVkFontTexture(SolVkState *vkstate)
{
    SolResource res = Sol_LoadResource("ID_FONT_ATLAS");

    SolResource metrics = Sol_LoadResource("ID_FONT_METRICS");
    if (metrics.data)
        Sol_ParseFontMetrics(vkstate, (const char *)metrics.data, 224.0f, 224.0f);

    printf("font atlas load: data=%p size=%zu\n", res.data, res.size);
    if (!res.data)
        return 1;

    // we need to know width/height — hardcode or store in resource
    uint32_t texW = 224, texH = 224;

    // --- staging buffer ---
    VkBuffer staging;
    VkDeviceMemory stagingMem;
    SolCreateBuffer(vkstate,res.size,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &staging, &stagingMem);

    void *mapped;
    vkMapMemory(vkstate->device, stagingMem, 0, res.size, 0, &mapped);
    memcpy(mapped, res.data, res.size);
    vkUnmapMemory(vkstate->device, stagingMem);

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
    vkCreateImage(vkstate->device, &imageInfo, NULL, &vkstate->fontImage);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkstate->device, vkstate->fontImage, &memReqs);
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = SolFindMemoryType(vkstate->physicalDevice,
                                                  memReqs.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(vkstate->device, &allocInfo, NULL, &vkstate->fontMemory);
    vkBindImageMemory(vkstate->device, vkstate->fontImage, vkstate->fontMemory, 0);

    // --- transition + copy via one-time command buffer ---
    VkCommandPool transferPool;
    VkCommandPoolCreateInfo transferPoolInfo = {0};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = vkstate->graphicsQueueFamily;
    transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    vkCreateCommandPool(vkstate->device, &transferPoolInfo, NULL, &transferPool);

    VkCommandBufferAllocateInfo cmdAlloc = {0};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = transferPool; // use transferPool not commandPool
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vkstate->device, &cmdAlloc, &cmd);

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
    toTransfer.image = vkstate->fontImage;
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
    vkCmdCopyBufferToImage(cmd, staging, vkstate->fontImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // transition to shader read
    VkImageMemoryBarrier toRead = {0};
    toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.image = vkstate->fontImage;
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
    vkQueueSubmit(vkstate->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(vkstate->graphicsQueue);
    vkDestroyBuffer(vkstate->device, staging, NULL);
    vkFreeMemory(vkstate->device, stagingMem, NULL);
    vkDestroyCommandPool(vkstate->device, transferPool, NULL); // frees cmd implicitly

    // --- image view ---
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkstate->fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(vkstate->device, &viewInfo, NULL, &vkstate->fontImageView);

    // --- sampler ---
    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(vkstate->device, &samplerInfo, NULL, &vkstate->fontSampler);

    return 0;
}

int SolVkFontDescriptors(SolVkState *vkstate)
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
    vkCreateDescriptorSetLayout(vkstate->device, &layoutInfo, NULL, &vkstate->fontDescLayout);

    // --- pool ---
    VkDescriptorPoolSize poolSize = {0};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(vkstate->device, &poolInfo, NULL, &vkstate->fontDescPool);

    // --- allocate set ---
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vkstate->fontDescPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &vkstate->fontDescLayout;
    vkAllocateDescriptorSets(vkstate->device, &allocInfo, &vkstate->fontDescSet);

    // --- write ---
    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.sampler = vkstate->fontSampler;
    imageInfo.imageView = vkstate->fontImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {0};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vkstate->fontDescSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(vkstate->device, 1, &write, 0, NULL);

    return 0;
}

int SolVkImageViews(SolVkState *vkstate)
{
    for (uint32_t i = 0; i < vkstate->swapchainImageCount; i++)
    {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vkstate->swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vkstate->swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(vkstate->device, &createInfo, NULL, &vkstate->swapchainImageViews[i]);
        if (result != VK_SUCCESS)
        {
            Sol_MessageBox("Failed to create image view", "Error");
            // MessageBox(NULL, "Failed to create image view", "Error", MB_OK);
            return 1;
        }
    }

    return 0;
}

int SolVkPipeline(SolVkState *vkstate,
                  SolPipelineConfig pipeConfig,
                  VkPipeline *outPipeline,
                  VkPipelineLayout *outLayout)
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
    vkCreateShaderModule(vkstate->device, &vertModuleInfo, NULL, &vertModule);
    vkCreateShaderModule(vkstate->device, &fragModuleInfo, NULL, &fragModule);

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
    vkCreatePipelineLayout(vkstate->device, &pipelineLayoutInfo, NULL, outLayout);

    // --- dynamic rendering info (replaces render pass) ---
    VkPipelineRenderingCreateInfo renderingInfo = {0};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &vkstate->swapchainImageFormat;
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

    VkResult result = vkCreateGraphicsPipelines(vkstate->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, outPipeline);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create graphics pipeline", "Error");
        return 1;
    }

    // shader modules no longer needed after pipeline creation
    vkDestroyShaderModule(vkstate->device, vertModule, NULL);
    vkDestroyShaderModule(vkstate->device, fragModule, NULL);

    return 0;
}

int SolVkCommandPool(SolVkState *vkstate)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vkstate->graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(vkstate->device, &poolInfo, NULL, &vkstate->commandPool);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create command pool", "Error");
        // MessageBox(NULL, "Failed to create command pool", "Error", MB_OK);
        return 1;
    }

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkstate->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    result = vkAllocateCommandBuffers(vkstate->device, &allocInfo, vkstate->commandBuffers);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to allocate command buffers", "Error");
        // MessageBox(NULL, "Failed to allocate command buffers", "Error", MB_OK);
        return 1;
    }

    return 0;
}

int SolVkSyncObjects(SolVkState *vkstate)
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame doesn't wait forever

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vkstate->device,
                              &semaphoreInfo, NULL,
                              &vkstate->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vkstate->device,
                              &semaphoreInfo, NULL,
                              &vkstate->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkstate->device, &fenceInfo, NULL, &vkstate->inFlightFences[i]) != VK_SUCCESS)
        {
            Sol_MessageBox("Failed to create sync objects", "Error");
            // MessageBox(NULL, "Failed to create sync objects", "Error", MB_OK);
            return 1;
        }
    }

    return 0;
}

int SolVkSSBO(SolVkState *vkstate)
{
    // 1. Create Descriptor Set Layout for the SSBO
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding};
    vkCreateDescriptorSetLayout(vkstate->device, &layoutInfo, NULL, &vkstate->modelDescLayout);

    // 2. Create Descriptor Pool
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT};

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize};
    vkCreateDescriptorPool(vkstate->device, &poolInfo, NULL, &vkstate->modelDescPool);

    // 3. Allocate and Update Sets for each frame
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // Create the Buffer for this frame
        SolCreateBuffer(vkstate,
            sizeof(ModelInstanceData) * MAX_MODEL_INSTANCES,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &vkstate->modelBuffer[i], &vkstate->modelMemory[i]); // Note: Should probably use separate memory handles or offsets

        vkMapMemory(vkstate->device, vkstate->modelMemory[i], 0, VK_WHOLE_SIZE, 0, &vkstate->modelDataPtr[i]);

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = vkstate->modelDescPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &vkstate->modelDescLayout};

        // You'll need an array: VkDescriptorSet instanceDescSets[MAX_FRAMES_IN_FLIGHT];
        vkAllocateDescriptorSets(vkstate->device, &allocInfo, &vkstate->modelDescSet[i]);

        VkDescriptorBufferInfo bufferInfo = {
            .buffer = vkstate->modelBuffer[i],
            .offset = 0,
            .range = sizeof(ModelInstanceData) * MAX_MODEL_INSTANCES};

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vkstate->modelDescSet[i],
            .dstBinding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo};
        vkUpdateDescriptorSets(vkstate->device, 1, &write, 0, NULL);
    }
    return 0;
}

uint32_t SolFindMemoryType(VkPhysicalDevice physicalDevice,
                           uint32_t typeFilter,
                           VkMemoryPropertyFlags properties)
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

int SolCreateBuffer(SolVkState *vkstate,
                    VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer *outBuffer, VkDeviceMemory *outMemory)
{
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkstate->device, &bufferInfo, NULL, outBuffer) != VK_SUCCESS)
    {
        printf("Failed to create buffer\n");
        return 1;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vkstate->device, *outBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = SolFindMemoryType(vkstate->physicalDevice, memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(vkstate->device, &allocInfo, NULL, outMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate buffer memory\n");
        return 1;
    }

    vkBindBufferMemory(vkstate->device, *outBuffer, *outMemory, 0);
    return 0;
}

int Sol_Pipeline_BuildAll(SolVkState *vkstate)
{
    pipelineConfigs[PIPE_3D_MESH].descLayouts = &vkstate->modelDescLayout;
    pipelineConfigs[PIPE_2D_TEXT].descLayouts = &vkstate->fontDescLayout;

    for (int i = 0; i < PIPE_COUNT; ++i)
    {
        if (SolVkPipeline(vkstate,
                          pipelineConfigs[i],
                          &vkstate->pipeline[i],
                          &vkstate->pipelineLayout[i]) != 0)
            return 1;
    }
    return 0;
}
