#include "includes/render.h"
#include <assert.h>
#include <vulkan/vulkan.h>

#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16

static VkInstance instance = VK_NULL_HANDLE;
static VkSurfaceKHR surface = VK_NULL_HANDLE;
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static uint32_t graphicsQueueFamily = 0;
static VkDevice device = VK_NULL_HANDLE;
static VkQueue graphicsQueue = VK_NULL_HANDLE;

// forward declare static functions at the top
static VkResult SolVkInstance();
static int SolVkSurface(HWND hwnd, HINSTANCE hInstance);
static int SolVkPhysicalDevice();
static int SolVkDevice();

void Sol_Init_Vulkan(HWND hwnd, HINSTANCE hInstance)
{
    VkResult result = SolVkInstance();
    if (result != VK_SUCCESS) return;
    if (SolVkSurface(hwnd, hInstance) != 0) return;
    if (SolVkPhysicalDevice() != 0) return;
    if (SolVkDevice() != 0) return;
}

static VkResult SolVkInstance()
{
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    VkApplicationInfo appInfo = {0};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Sol Vulk";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = 2;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        MessageBox(NULL, "Failed to create Vulkan instance", "Error", MB_OK);
    }
    return result;
}

static int SolVkSurface(HWND hwnd, HINSTANCE hInstance)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {0};
    surfaceCreateInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd      = hwnd;
    surfaceCreateInfo.hinstance = hInstance;

    VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
    if (result != VK_SUCCESS) {
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
    if (deviceCount == 0) {
        MessageBox(NULL, "No Vulkan capable GPU found", "Error", MB_OK);
        return 1;
    }
    assert(deviceCount <= MAX_DEVICE_QUERY);
    // get the actual devices
    VkPhysicalDevice devices[8]; // 8 is plenty
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // pick the first discrete GPU we find, fallback to first device
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physicalDevice = devices[i];
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        physicalDevice = devices[0]; // fallback
    }

    // find a queue family that supports graphics
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    
    assert(queueFamilyCount <= MAX_QUEUE_FAMILIES);
    VkQueueFamilyProperties queueFamilies[16];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
            break;
        }
    }

    VkPhysicalDeviceProperties selectedProps;
    vkGetPhysicalDeviceProperties(physicalDevice, &selectedProps);
    MessageBox(NULL, selectedProps.deviceName, "GPU Selected", MB_OK);

    return 0;
}

static int SolVkDevice()
{
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char *deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = 1;
    createInfo.pQueueCreateInfos       = &queueCreateInfo;
    createInfo.enabledExtensionCount   = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
    if (result != VK_SUCCESS) {
        MessageBox(NULL, "Failed to create logical device", "Error", MB_OK);
        return 1;
    }

    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    MessageBox(NULL, "Logical device created!", "Success", MB_OK);
    return 0;
}