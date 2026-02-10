#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "gfx/vulkan/validation.h"
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "context.h"

namespace
{
struct VulkanInstanceConfig
{
    const char* appName;
    uint32_t appVersion;
    uint32_t apiVersion;
    bool enableValidation;
};

VkInstance createVulkanInstance(VulkanInstanceConfig& config)
{
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config.appName,
        .apiVersion = config.apiVersion,
    };

    uint32_t instanceExtensionCount{ 0 };
    char const* const* instanceExtensions =
        SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions,
    };

    VkInstance instance{ VK_NULL_HANDLE };
    vkCreateInstance(&instanceCI, nullptr, &instance);
    return instance;
}

VkSurfaceKHR createVulkanSurface(VkInstance instance, const Window& window)
{
    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    SDL_Vulkan_CreateSurface(
        window.getSDLWindow(),
        instance,
        nullptr,
        &surface
    );
    return surface;
}
} // namespace

void VulkanContext::init(VulkanContextConfig& config)
{
    VulkanInstanceConfig instanceConfig{
        .appName = config.appName,
        .appVersion = config.appVersion,
        .apiVersion = config.apiVersion,
        .enableValidation = config.enableValidation,
    };

    m_instance = createVulkanInstance(instanceConfig);
    if (config.enableValidation)
    {
        m_debugMessenger = createDebugMessenger(m_instance);
    }
}

void VulkanContext::shutdown()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::createSurface(const Window& window)
{
    m_surface = createVulkanSurface(m_instance, window);
}

VkInstance VulkanContext::getInstance() const
{
    return m_instance;
}
