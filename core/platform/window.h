#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
class Window
{
  public:
    SDL_Window* window;
    glm::ivec2 windowSize;
};
