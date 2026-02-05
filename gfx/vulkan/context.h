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

class VulkanContext
{
  public:
    VkInstance instance;
    VkDevice device;
    std::vector<VkPhysicalDevice> devices;
    VkQueue queue;
    VkCommandPool commandPool;
    VmaAllocator allocator;
    VkSurfaceKHR surface;

  private:
    void createInstance(SDL_Window* window);
    void selectPhysicalDevice(uint32_t deviceIndex);
    void createDevice();
    void createAllocator();
    void createCommandPool();
};
