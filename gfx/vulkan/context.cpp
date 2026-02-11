#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include "gfx/vulkan/validation.h"
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "context.h"

void VulkanContext::createInstance(VulkanInstanceConfig& config)
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

    vkCreateInstance(&instanceCI, nullptr, &m_instance);
}

void VulkanContext::createSurface(const Window& window)
{
    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    SDL_Vulkan_CreateSurface(
        window.getSDLWindow(),
        m_instance,
        nullptr,
        &m_surface
    );
}

void VulkanContext::selectPhysicalDevice()
{
    uint32_t deviceCount{ 0 };
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            m_physicalDevice = device;
            std::cout << "Selected: " << props.deviceName << '\n';
            break;
        }
    }
    // fallback to first compatible device
    if (!m_physicalDevice)
    {
        m_physicalDevice = devices[0];
    }
    m_queueFamily = findQueueFamily();
}

uint32_t VulkanContext::findQueueFamily()
{
    uint32_t queueFamilyCount{ 0 };
    assert(m_physicalDevice);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physicalDevice,
        &queueFamilyCount,
        nullptr
    );
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physicalDevice,
        &queueFamilyCount,
        queueFamilies.data()
    );
    uint32_t queueFamily{ 0 };
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            SDL_Vulkan_GetPresentationSupport(m_instance, m_physicalDevice, i))
        {
            queueFamily = i;
            break;
        }
    }
    return queueFamily;
}

void VulkanContext::init(VulkanInstanceConfig& config, const Window& window)
{
    createInstance(config);
    assert(m_instance);
    if (config.enableValidation)
    {
        m_debugMessenger = createDebugMessenger(m_instance);
    }
    assert(m_debugMessenger);
    createSurface(window);
    assert(m_surface);
    selectPhysicalDevice();
    assert(m_physicalDevice);
}

void VulkanContext::createLogicalDevice()
{
    const float queueFamilyPriorities{ 1.0f };
    VkDeviceQueueCreateInfo queueCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = m_queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queueFamilyPriorities,
    };
    VkPhysicalDeviceVulkan12Features enabledVk12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE
    };
    VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE
    };
    const std::vector<const char*> deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const VkPhysicalDeviceFeatures enabledVk10Features{ .samplerAnisotropy =
                                                            VK_TRUE };
    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledVk13Features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features
    };
    vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_logicalDevice);
    vkGetDeviceQueue(m_logicalDevice, m_queueFamily, 0, &m_queue);
}

void VulkanContext::shutdown()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    destroyDebugMessenger(m_instance, m_debugMessenger);
    vkDestroyInstance(m_instance, nullptr);
}

VkInstance VulkanContext::getInstance() const
{
    return m_instance;
}
