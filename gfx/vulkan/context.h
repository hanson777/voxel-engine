#pragma once

// We still need this define here so that any file including
// this header knows not to use standard Vulkan prototypes.
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <SDL3/SDL.h>
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
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
    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    VmaAllocator m_allocator{ VK_NULL_HANDLE };

    void createInstance(VulkanInstanceConfig& config);
    void createSurface(const Window& window);
    void selectPhysicalDevice();
    uint32_t findQueueFamily();
    void createLogicalDevice();
    void createSwapchain();

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
