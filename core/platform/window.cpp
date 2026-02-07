#include "window.h"
#include <SDL3/SDL.h>

Window::Window(const WindowConfig& config)
{
    SDL_Init(SDL_INIT_VIDEO);
    m_shouldClose = false;
    m_window = SDL_CreateWindow(
        config.title,
        config.width,
        config.height,
        SDL_WINDOW_RESIZABLE
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
            break;
        }
    }
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
