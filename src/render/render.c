#define VK_USE_PLATFORM_WIN32_KHR
#include "render.h"
#include <windows.h>
#include "vulkan/vulkan.h"

VkInstance instance;

void init_vulkan(HWND hwnd)
{
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "SolVulk",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0};

    const char *extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"};

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = extensions};

    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
    {
        // Handle error (e.g., MessageBox)
    }

    VkSurfaceKHR surface;
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hwnd = hwnd,
        .hinstance = GetModuleHandle(NULL)};

    if (vkCreateWin32SurfaceKHR(instance, &surfaceInfo, NULL, &surface) != VK_SUCCESS)
    {
        // Handle error
    }
}