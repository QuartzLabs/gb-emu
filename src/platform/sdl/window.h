#pragma once

#include <SDL.h>
#include <string>

namespace gb::platform::sdl
{
    class Window
    {
    public:
        Window(const char* title, int width, int height);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool is_valid() const;

        void clear();
        void present();
        void draw_ui(bool rom_loaded, const std::string& rom_name);

        SDL_Window* native_window() const;
        SDL_Renderer* native_renderer() const;

    private:
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
        int m_width;
        int m_height;
    };
}