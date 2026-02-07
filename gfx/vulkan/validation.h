#pragma once

#include <volk/volk.h>

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);

void destroyDebugMessenger(
    VkInstance instance, VkDebugUtilsMessengerEXT messenger
);
