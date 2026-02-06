#include "window.h"
#include <SDL3/SDL.h>

Window::Window(int width, int height) : m_width(width), m_height(height)
{
    m_window = SDL_CreateWindow(
        "Dead Voxel",
        width,
        height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );
}

Window::~Window()
{
    SDL_DestroyWindow(m_window);
}

SDL_Window* Window::getSDLWindow() const
{
    return m_window;
}

void Window::pollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            m_shouldClose = true;
        }
    }
    m_shouldClose = false;
}

bool Window::shouldClose()
{
    return m_shouldClose;
}

int Window::width() const
{
    return m_width;
}

int Window::height() const
{
    return m_height;
}
