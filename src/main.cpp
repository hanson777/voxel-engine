#include "../core/platform/window.h"
#include <iostream>

int main()
{
    std::cout << "We are all alone on life's journey, held captive by the "
                 "limitations of human consciousness.\n";

    Window window = Window(720, 1280);
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO);

    while (!done)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
        }

        // Do game logic, present a frame, etc.
    }
    return 0;
}
