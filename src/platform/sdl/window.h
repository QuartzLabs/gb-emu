#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdint>
#include <string>

#include "core/cpu/cpu.h"

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
        void draw_ui(
            bool rom_loaded,
            const std::string& rom_name,
            const gb::Cpu& cpu
        );

        SDL_Window* native_window() const;
        SDL_Renderer* native_renderer() const;

    private:
        void draw_text(const std::string& text, int x, int y, SDL_Color color) const;
        std::string hex8(unsigned int value) const;
        std::string hex16(unsigned int value) const;
        std::string hex64(std::uint64_t value) const;
        std::string memory_row(const gb::Bus* bus, unsigned int start, unsigned int pc) const;

    private:
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
        TTF_Font* m_font;
        int m_width;
        int m_height;
    };
}