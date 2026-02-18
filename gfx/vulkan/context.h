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
constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

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
    void destroyImages(VkDevice device, VmaAllocator allocator)
    {
        for (auto view : imageViews)
        {
            if (view)
            {
                vkDestroyImageView(device, view, nullptr);
            }
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
    }
    void cleanup(VkDevice device, VmaAllocator allocator)
    {
        destroyImages(device, allocator);
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
    VmaAllocator m_allocator{ VK_NULL_HANDLE };
    Swapchain m_swapchain{};
    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_fences;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_presentationSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_renderSemaphores;
    uint32_t m_currentFrame{ 0 };

    // Instance
    void createInstance();

    // Surface
    void createSurface(const Window& window);

    // Device selection
    void selectPhysicalDevice();
    uint32_t findQueueFamily();
    void createLogicalDevice();

    // VMA Allocator
    void createAllocator();

    // Swapchain
    void createSwapchain(
        const Window& window, const VkSwapchainKHR oldSwapchainHandle
    );
    void recreateSwapchain(const Window& window);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );
    VkPresentModeKHR
    chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
    VkExtent2D chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities, const Window& window
    );

    // Depth attachment
    void createDepthResources();
    VkFormat findDepthFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    // Command pool and command buffers
    void createCommandPool();
    void createCommandBuffers();

    // Fences and semaphores
    void createSyncObjects();

    // Frame logic
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void transitionImageLayout(
        VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout,
        VkImageLayout newLayout
    );

    void shutdown();

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

    void init(const Window& window);

    void beginFrame(const Window& window);
    void endFrame();

    VkInstance getInstance() const;
};
