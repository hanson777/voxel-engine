#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include <iostream>
#include "gfx/vulkan/validation.h"
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vma/vk_mem_alloc.h>
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
        VkBool32 presentationSupport{ false };
        vkGetPhysicalDeviceSurfaceSupportKHR(
            m_physicalDevice,
            i,
            m_surface,
            &presentationSupport
        );
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            presentationSupport)
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
    createLogicalDevice();
    assert(m_logicalDevice);
    createSwapchain(window);
    assert(m_swapchain);
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
        .pNext = nullptr,
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

VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats
)
{
    // Prefer BGRA8 SRGB for proper gamma correction
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    // Fallback to RGBA8 SRGB if BGRA not available
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    // Last resort: just use the first available format
    return availableFormats[0];
}

VkPresentModeKHR VulkanContext::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availableModes
)
{
    // Prefer MAILBOX (triple buffering, low latency, no tearing)
    for (const auto& availableMode : availableModes)
    {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availableMode;
        }
    }

    // FIFO is guaranteed to be available (V-Sync)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanContext::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities, const Window& window
)
{
    // If currentExtent is not UINT32_MAX, the window manager dictates the size
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    // Otherwise, we pick the resolution ourselves based on window size
    int width, height;
    SDL_GetWindowSizeInPixels(window.getSDLWindow(), &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height) };

    // Clamp to min/max supported by surface
    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return actualExtent;
}

void VulkanContext::createSwapchain(const Window& window)
{
    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_physicalDevice,
        m_surface,
        &capabilities
    );

    // Query supported formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physicalDevice,
        m_surface,
        &formatCount,
        nullptr
    );
    assert(formatCount > 0);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physicalDevice,
        m_surface,
        &formatCount,
        formats.data()
    );

    // Query supported present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physicalDevice,
        m_surface,
        &presentModeCount,
        nullptr
    );
    assert(presentModeCount > 0);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physicalDevice,
        m_surface,
        &presentModeCount,
        presentModes.data()
    );

    // Choose best options
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
    VkExtent2D extent = chooseSwapExtent(capabilities, window);

    // Choose image count (prefer minImageCount + 1 for triple buffering)
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // 6. Create swapchain
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1, // Always 1 unless doing stereoscopic 3D
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode =
            VK_SHARING_MODE_EXCLUSIVE, // We use single queue family
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE, // Don't render pixels obscured by other windows
        .oldSwapchain = VK_NULL_HANDLE
    };

    vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapchain);

    // 7. Retrieve swapchain images
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(
        m_logicalDevice,
        m_swapchain,
        &imageCount,
        m_swapchainImages.data()
    );

    // 8. Store format and extent for later use
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;

    // 9. Create image views for each swapchain image
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (size_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_swapchainImageFormat,
            .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .baseMipLevel = 0,
                                  .levelCount = 1,
                                  .baseArrayLayer = 0,
                                  .layerCount = 1 }
        };

        vkCreateImageView(
            m_logicalDevice,
            &viewInfo,
            nullptr,
            &m_swapchainImageViews[i]
        );
    }

    std::cout << "Swapchain created: " << imageCount << " images, "
              << extent.width << "x" << extent.height << '\n';
}

void VulkanContext::shutdown()
{
    // Destroy swapchain image views
    for (auto imageView : m_swapchainImageViews)
    {
        vkDestroyImageView(m_logicalDevice, imageView, nullptr);
    }
    m_swapchainImageViews.clear();

    // Destroy swapchain
    if (m_swapchain)
    {
        vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
    }

    // Destroy logical device
    if (m_logicalDevice)
    {
        vkDestroyDevice(m_logicalDevice, nullptr);
    }

    // Destroy surface
    if (m_surface)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }

    // Destroy debug messenger
    if (m_debugMessenger)
    {
        destroyDebugMessenger(m_instance, m_debugMessenger);
    }

    // Destroy instance
    if (m_instance)
    {
        vkDestroyInstance(m_instance, nullptr);
    }
}

VkInstance VulkanContext::getInstance() const
{
    return m_instance;
}
