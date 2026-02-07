#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

struct WindowConfig
{
    int width;
    int height;
    const char* title;
};

class Window
{
  private:
    SDL_Window* m_window;
    int m_width;
    int m_height;
    bool m_shouldClose;

  public:
    Window(const WindowConfig& config);
    ~Window();

    SDL_Window* getSDLWindow() const;
    void pollEvents();
    bool shouldClose();

    int width() const;
    int height() const;
};
