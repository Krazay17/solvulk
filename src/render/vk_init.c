#include "sol_core.h"
#include "render.h"

VkResult SolVkInstance(SolVkState *vkstate)
{
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    VkApplicationInfo appInfo  = {0};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = NULL;
    appInfo.pApplicationName   = "Sol Vulk";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo    = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = 2;
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
    surfaceCreateInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd                        = hwnd;
    surfaceCreateInfo.hinstance                   = hInstance;

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
    vkGetPhysicalDeviceQueueFamilyProperties(vkstate->physicalDevice, &queueFamilyCount, NULL);
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
    queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex        = vkstate->graphicsQueueFamily;
    queueCreateInfo.queueCount              = 1;
    queueCreateInfo.pQueuePriorities        = &queuePriority;

    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature = {0};
    dynamicRenderingFeature.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo createInfo      = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = 1;
    createInfo.pQueueCreateInfos       = &queueCreateInfo;
    createInfo.enabledExtensionCount   = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.pNext                   = &dynamicRenderingFeature;

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
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width      = vkstate->swapchainExtent.width;
    imageInfo.extent.height     = vkstate->swapchainExtent.height;
    imageInfo.extent.depth      = 1;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.format            = depthFormat;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(vkstate->device, &imageInfo, NULL, &vkstate->depthImage);

    // 2. Memory Requirements & Allocation
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkstate->device, vkstate->depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memReqs.size;
    if (SolFindMemoryType(vkstate->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &allocInfo.memoryTypeIndex) != 0)
        return 1;

    vkAllocateMemory(vkstate->device, &allocInfo, NULL, &vkstate->depthMemory);
    vkBindImageMemory(vkstate->device, vkstate->depthImage, vkstate->depthMemory, 0);

    // 3. Create View
    VkImageViewCreateInfo viewInfo           = {0};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = vkstate->depthImage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = depthFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

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
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkstate->physicalDevice, vkstate->surface, &presentModeCount,
                                              presentModes);

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
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                  = vkstate->surface;
    createInfo.minImageCount            = capabilities.minImageCount + 1; // one extra to avoid waiting
    createInfo.imageFormat              = chosenFormat.format;
    createInfo.imageColorSpace          = chosenFormat.colorSpace;
    createInfo.imageExtent              = vkstate->swapchainExtent;
    createInfo.imageArrayLayers         = 1;
    createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform             = capabilities.currentTransform;
    createInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode              = chosenPresentMode;
    createInfo.clipped                  = VK_TRUE;

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
    vkGetSwapchainImagesKHR(vkstate->device, vkstate->swapchain, &vkstate->swapchainImageCount,
                            vkstate->swapchainImages);

    return 0;
}

int SolVkImageViews(SolVkState *vkstate)
{
    for (uint32_t i = 0; i < vkstate->swapchainImageCount; i++)
    {
        VkImageViewCreateInfo createInfo           = {0};
        createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                           = vkstate->swapchainImages[i];
        createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                          = vkstate->swapchainImageFormat;
        createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

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

int SolVkCommandPool(SolVkState *vkstate)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex        = vkstate->graphicsQueueFamily;
    poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(vkstate->device, &poolInfo, NULL, &vkstate->commandPool);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create command pool", "Error");
        return 1;
    }

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = vkstate->commandPool;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = MAX_FRAMES_IN_FLIGHT;

    result = vkAllocateCommandBuffers(vkstate->device, &allocInfo, vkstate->commandBuffers);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to allocate command buffers", "Error");
        return 1;
    }

    return 0;
}

int SolVkSyncObjects(SolVkState *vkstate)
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame doesn't wait forever

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vkstate->device, &semaphoreInfo, NULL, &vkstate->imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateSemaphore(vkstate->device, &semaphoreInfo, NULL, &vkstate->renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateFence(vkstate->device, &fenceInfo, NULL, &vkstate->inFlightFences[i]) != VK_SUCCESS)
        {
            Sol_MessageBox("Failed to create sync objects", "Error");
            return 1;
        }
    }

    return 0;
}

int SolFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties,
                      uint32_t *outIndex)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            *outIndex = i;
            return 0;
        }
    }

    return 1;
}

int SolCreateBuffer(SolVkState *vkstate, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer *outBuffer, VkDeviceMemory *outMemory)
{
    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(vkstate->device, &bufferInfo, NULL, outBuffer) != VK_SUCCESS)
        return 1;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vkstate->device, *outBuffer, &memReqs);

    uint32_t memIndex;
    if (SolFindMemoryType(vkstate->physicalDevice, memReqs.memoryTypeBits, properties, &memIndex) != 0)
    {
        vkDestroyBuffer(vkstate->device, *outBuffer, NULL);
        return 1;
    }

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memReqs.size,
        .memoryTypeIndex = memIndex,
    };

    if (vkAllocateMemory(vkstate->device, &allocInfo, NULL, outMemory) != VK_SUCCESS)
    {
        vkDestroyBuffer(vkstate->device, *outBuffer, NULL);
        return 1;
    }

    vkBindBufferMemory(vkstate->device, *outBuffer, *outMemory, 0);
    return 0;
}

int Sol_CreateFrameBuffer(SolVkState *vkstate, VkDeviceSize size, VkBufferUsageFlags usage, SolFrameBuffer *out)
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (SolCreateBuffer(vkstate, size, usage,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &out->buffers[i], &out->memory[i]) != 0)
            return 1;

        vkMapMemory(vkstate->device, out->memory[i], 0, VK_WHOLE_SIZE, 0, &out->mapped[i]);
    }
    return 0;
}

int Sol_CreateDescriptorImage(SolVkState *vkstate, VkImageView imageView, VkSampler sampler,
                              VkShaderStageFlags stageFlags, SolDescriptor *out)
{
    VkDescriptorSetLayoutBinding binding = {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags      = stageFlags,
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &binding,
    };
    vkCreateDescriptorSetLayout(vkstate->device, &layoutInfo, NULL, &out->layout);

    VkDescriptorPoolSize poolSize = {
        .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize,
    };
    vkCreateDescriptorPool(vkstate->device, &poolInfo, NULL, &out->pool);

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = out->pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &out->layout,
    };

    VkDescriptorSet set;
    vkAllocateDescriptorSets(vkstate->device, &allocInfo, &set);

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT;i++)
    {
        out->sets[i] = set;
    }

    VkDescriptorImageInfo imageInfo = {
        .sampler     = sampler,
        .imageView   = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set,
        .dstBinding      = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo      = &imageInfo,
    };
    vkUpdateDescriptorSets(vkstate->device, 1, &write, 0, NULL);

    return 0;
}
