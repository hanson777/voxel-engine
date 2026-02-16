#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include <iostream>
#include <stdexcept>
#include "gfx/vulkan/validation.h"
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "context.h"

void VulkanContext::createInstance()
{
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "App",
        .apiVersion = VULKAN_API_VERSION,
    };

    // Get SDL extensions and add debug utils extension
    uint32_t sdlExtensionCount{ 0 };
    char const* const* sdlExtensions =
        SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    std::vector<const char*> extensions(
        sdlExtensions,
        sdlExtensions + sdlExtensionCount
    );
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef __APPLE__
    // Required for MoltenVK on macOS
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    // Enable validation layer
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &validationLayer,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkResult result = vkCreateInstance(&instanceCI, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
        std::cerr << "vkCreateInstance failed with error code: " << result
                  << '\n';
        throw std::runtime_error("failed to create Vulkan instance");
    }
}

void VulkanContext::createSurface(const Window& window)
{
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
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            m_physicalDevice = device;
            std::cout << "Selected: " << properties.deviceName << '\n';
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
        .pNext = nullptr,              // always good practice to init this
        .descriptorIndexing = VK_TRUE, // essential for array of textures
        .runtimeDescriptorArray =
            VK_TRUE, // essential for GPU pointers / ray tracing
        .bufferDeviceAddress = VK_TRUE
    };
    VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features, // chain the 1.2 struct
        .synchronization2 = VK_TRUE,   // better barriers
        .dynamicRendering = VK_TRUE    // no render passes
    };
    const std::vector<const char*> deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_portability_subset",
#endif
    };

    const VkPhysicalDeviceFeatures enabledVk10Features{
        .fillModeNonSolid = VK_TRUE,  // wireframe
        .samplerAnisotropy = VK_TRUE, // sharp textures at angles
    };

    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledVk13Features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features
    };
    vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_device);
    vkGetDeviceQueue(m_device, m_queueFamily, 0, &m_queue);
}

void VulkanContext::createAllocator()
{
    VmaVulkanFunctions vulkanFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo allocatorCI{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_physicalDevice,
        .device = m_device,
        .pVulkanFunctions = &vulkanFunctions,
        .instance = m_instance,
        .vulkanApiVersion = VULKAN_API_VERSION,
    };

    if (vmaCreateAllocator(&allocatorCI, &m_allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create VMA allocator");
    }
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
    // for (const auto& availableMode : availableModes)
    // {
    //     if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
    //     {
    //         return availableMode;
    //     }
    // }

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

void VulkanContext::createSwapchain(
    const Window& window, const VkSwapchainKHR oldSwapchainHandle
)
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
    VkSwapchainCreateInfoKHR swapchainCI{
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
        .oldSwapchain = oldSwapchainHandle,
    };

    vkCreateSwapchainKHR(m_device, &swapchainCI, nullptr, &m_swapchain.handle);

    // Retrieve swapchain images
    vkGetSwapchainImagesKHR(m_device, m_swapchain.handle, &imageCount, nullptr);
    m_swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(
        m_device,
        m_swapchain.handle,
        &imageCount,
        m_swapchain.images.data()
    );

    // Store format and extent for later use
    m_swapchain.imageFormat = surfaceFormat.format;
    m_swapchain.extent = extent;

    // Create image views for each swapchain image
    m_swapchain.imageViews.resize(m_swapchain.images.size());
    for (size_t i = 0; i < m_swapchain.images.size(); i++)
    {
        VkImageViewCreateInfo imageViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapchain.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_swapchain.imageFormat,
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
            m_device,
            &imageViewCI,
            nullptr,
            &m_swapchain.imageViews[i]
        );
    }

    std::cout << "Swapchain created: " << imageCount << " images, "
              << extent.width << "x" << extent.height << '\n';
}

void VulkanContext::recreateSwapchain(const Window& window)
{
    int width, height;
    SDL_GetWindowSizeInPixels(window.getSDLWindow(), &width, &height);
    while (width == 0 || height == 0)
    {
        SDL_GetWindowSizeInPixels(window.getSDLWindow(), &width, &height);
        SDL_WaitEvent(nullptr);
    }
    vkDeviceWaitIdle(m_device);
    m_swapchain.destroyImages(m_device, m_allocator);
    VkSwapchainKHR oldHandle = m_swapchain.handle;
    m_swapchain.handle = VK_NULL_HANDLE;
    createSwapchain(window, oldHandle);
    createDepthResources();
    vkDestroySwapchainKHR(m_device, oldHandle, nullptr);
}

VkFormat VulkanContext::findSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features
)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

void VulkanContext::createDepthResources()
{
    // Choose the format (D32 is best, D24 is fallback)
    VkFormat depthFormat = findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT,
          VK_FORMAT_D32_SFLOAT_S8_UINT,
          VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    m_swapchain.depthFormat = depthFormat;

    VkImageCreateInfo imageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent = { .width = m_swapchain.extent.width,
                    .height = m_swapchain.extent.height,
                    .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    vmaCreateImage(
        m_allocator,
        &imageCI,
        &allocCI,
        &m_swapchain.depthImage,
        &m_swapchain.depthImageAllocation,
        nullptr
    );

    VkImageViewCreateInfo imageViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_swapchain.depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                              .baseMipLevel = 0,
                              .levelCount = 1,
                              .baseArrayLayer = 0,
                              .layerCount = 1 },
    };

    if (vkCreateImageView(
            m_device,
            &imageViewCI,
            nullptr,
            &m_swapchain.depthImageView
        ) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create depth image view");
    }
}

void VulkanContext::createCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_queueFamily,
    };

    if (vkCreateCommandPool(
            m_device,
            &commandPoolCI,
            nullptr,
            &m_commandPool
        ) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool");
    }

    std::cout << "Command pool created\n";
}

void VulkanContext::createCommandBuffers()
{
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
    };

    VkResult result =
        vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    std::cout << "Command buffers allocated\n";
}

void VulkanContext::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                               .flags = VK_FENCE_CREATE_SIGNALED_BIT };

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkCreateFence(m_device, &fenceCI, nullptr, &m_fences[i]);
        vkCreateSemaphore(
            m_device,
            &semaphoreCI,
            nullptr,
            &m_presentationSemaphores[i]
        );
    }

    for (auto& semaphore : m_renderSemaphores)
    {
        vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &semaphore);
    }
}

void VulkanContext::init(const Window& window)
{
    volkInitialize();
    createInstance();
    assert(m_instance);
    volkLoadInstance(m_instance);
    m_debugMessenger = createDebugMessenger(m_instance);
    assert(m_debugMessenger);
    createSurface(window);
    assert(m_surface);
    selectPhysicalDevice();
    assert(m_physicalDevice);
    createLogicalDevice();
    assert(m_device);
    createAllocator();
    assert(m_allocator);
    createSwapchain(window, nullptr);
    assert(m_swapchain.handle);
    createDepthResources();
    assert(m_swapchain.depthImage);
    assert(m_swapchain.depthImageView);
    assert(m_swapchain.depthFormat);
    assert(m_swapchain.depthImageAllocation);
    createCommandPool();
    assert(m_commandPool);
    createCommandBuffers();
    for (auto& buffer : m_commandBuffers)
    {
        assert(buffer);
    }
    createSyncObjects();
    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        assert(m_fences[i]);
        assert(m_presentationSemaphores[i]);
        assert(m_renderSemaphores[i]);
    }
}

void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex)
{
}

void VulkanContext::beginFrame(const Window& window)
{
    vkWaitForFences(
        m_device,
        1,
        &m_fences[m_currentFrameInFlight],
        true,
        UINT64_MAX
    );
    vkResetFences(m_device, 1, &m_fences[m_currentFrameInFlight]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_device,
        m_swapchain.handle,
        UINT64_MAX,
        m_presentationSemaphores[m_currentFrameInFlight],
        VK_NULL_HANDLE,
        &imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain(window);
        return;
    }
}

void endFrame()
{
}

void VulkanContext::shutdown()
{
    // Destroy sync objects
    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device, m_presentationSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_fences[i], nullptr);
    }

    // Destroy command buffers
    if (m_commandBuffers.data())
    {
        vkFreeCommandBuffers(
            m_device,
            m_commandPool,
            m_commandBuffers.size(),
            m_commandBuffers.data()
        );
    }

    // Destroy command pool
    if (m_commandPool)
    {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }

    // Destroy swapchain (struct cleanup handles image views, depth, and
    // swapchain)
    m_swapchain.cleanup(m_device, m_allocator);

    // Destroy VMA allocator (must be before device destruction)
    if (m_allocator)
    {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    // Destroy logical device
    if (m_device)
    {
        vkDestroyDevice(m_device, nullptr);
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
