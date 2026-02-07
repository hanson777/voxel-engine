#include "../core/platform/window.h"
#include <iostream>

int main()
{
    std::cout << "We are all alone on life's journey, held captive by the "
                 "limitations of human consciousness.\n";

    WindowConfig windowConfig{
        .width = 720,
        .height = 480,
        .title = "V12",
    };
    Window window(windowConfig);

    while (!window.shouldClose())
    {
        window.pollEvents();
    }

    return 0;
}
