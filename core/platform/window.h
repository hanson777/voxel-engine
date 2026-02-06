#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

class Window
{
  private:
    SDL_Window* m_window;
    int m_width;
    int m_height;
    int m_shouldClose;

  public:
    Window(int width, int height);
    ~Window();

    SDL_Window* getSDLWindow() const;
    void pollEvents();
    bool shouldClose();

    int width() const;
    int height() const;
};
