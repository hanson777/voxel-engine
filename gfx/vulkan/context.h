#pragma once

// We still need this define here so that any file including
// this header knows not to use standard Vulkan prototypes.
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include "core/platform/window.h"

constexpr uint32_t VULKAN_API_VERSION{ VK_API_VERSION_1_3 };
constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 3 };

struct VulkanInstanceConfig
{
    const char* appName = "App";
    uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
    bool enableValidation = true;
};

struct Swapchain
{
    VkSwapchainKHR handle{ VK_NULL_HANDLE };

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkFormat imageFormat{ VK_FORMAT_UNDEFINED };
    VkExtent2D extent{};

    VkImage depthImage{ VK_NULL_HANDLE };
    VmaAllocation depthImageAllocation{ VK_NULL_HANDLE };
    VkImageView depthImageView{ VK_NULL_HANDLE };
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };

    void cleanup(VkDevice device, VmaAllocator allocator)
    {
        for (auto view : imageViews)
        {
            vkDestroyImageView(device, view, nullptr);
        }
        imageViews.clear();

        if (depthImageView)
        {
            vkDestroyImageView(device, depthImageView, nullptr);
            depthImageView = VK_NULL_HANDLE;
        }
        if (depthImage)
        {
            vmaDestroyImage(allocator, depthImage, depthImageAllocation);
            depthImage = VK_NULL_HANDLE;
            depthImageAllocation = VK_NULL_HANDLE;
        }

        if (handle)
        {
            vkDestroySwapchainKHR(device, handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }
};

class VulkanContext
{
  private:
    VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
    VkInstance m_instance{ VK_NULL_HANDLE };
    VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
    VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
    VkDevice m_device{ VK_NULL_HANDLE };
    uint32_t m_queueFamily{ 0 };
    VkQueue m_queue{ VK_NULL_HANDLE };
    Swapchain m_swapchain{};
    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers{};
    VmaAllocator m_allocator{ VK_NULL_HANDLE };

    void createInstance(VulkanInstanceConfig& config);

    // Surface
    void createSurface(const Window& window);

    // Device selection
    void selectPhysicalDevice();
    uint32_t findQueueFamily();
    void createLogicalDevice();

    // VMA Allocator
    void createAllocator();

    // Swapchain
    void createSwapchain(const Window& window);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );
    VkPresentModeKHR
    chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
    VkExtent2D chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities, const Window& window
    );

    // Depth attachment (apart of Swapchain)
    void createDepthResources();
    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    void createCommandPool();
    void createCommandBuffers();

  public:
    VulkanContext() = default;
    ~VulkanContext()
    {
        shutdown();
    }
    VulkanContext(VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    void init(VulkanInstanceConfig& config, const Window& window);
    void shutdown();

    VkInstance getInstance() const;
};
