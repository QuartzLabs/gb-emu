#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "platform/sdl/application.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    gb::platform::sdl::Application application;
    return application.run();
}