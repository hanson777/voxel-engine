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

struct VulkanInstanceConfig
{
    const char* appName = "App";
    uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t apiVersion = VK_API_VERSION_1_4;
    bool enableValidation = true;
};

class VulkanContext
{
  private:
    VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
    VkInstance m_instance{ VK_NULL_HANDLE };
    VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
    VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
    VkDevice m_logicalDevice{ VK_NULL_HANDLE };
    uint32_t m_queueFamily{ 0 };
    VkQueue m_queue{ VK_NULL_HANDLE };
    VkSwapchainKHR m_swapchain{ VK_NULL_HANDLE };
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainImageFormat{ VK_FORMAT_UNDEFINED };
    VkExtent2D m_swapchainExtent{};
    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    VmaAllocator m_allocator{ VK_NULL_HANDLE };

    void createInstance(VulkanInstanceConfig& config);

    // Surface
    void createSurface(const Window& window);

    // Device selection
    void selectPhysicalDevice();
    uint32_t findQueueFamily();
    void createLogicalDevice();

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
