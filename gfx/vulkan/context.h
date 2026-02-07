#pragma once

// We still need this define here so that any file including
// this header knows not to use standard Vulkan prototypes.
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <SDL3/SDL.h>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include "core/platform/window.h"

struct VulkanContextConfig
{
    const char* appName = "App";
    uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t apiVersion = VK_API_VERSION_1_4;
    bool enableValidation = true;
};

class VulkanContext
{
  private:
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkInstance m_instance;
    VkDevice m_device;
    std::vector<VkPhysicalDevice> m_devices;
    VkQueue m_queue;
    VkCommandPool m_commandPool;
    VmaAllocator m_allocator;
    VkSurfaceKHR m_surface;

  public:
    void init(VulkanContextConfig& config);
    void shutdown();

    void createSurface(const Window& window);

    VkInstance getInstance() const;
};
