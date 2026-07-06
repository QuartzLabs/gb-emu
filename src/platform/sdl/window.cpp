#include "window.h"

#include <stdexcept>
#include <string>

namespace gb::platform::sdl
{
Window::Window(const char* title, int width, int height)
    : m_window(nullptr)
    , m_renderer(nullptr)
    , m_width(width)
    , m_height(height)
{
    m_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN);

    if (!m_window)
    {
        throw std::runtime_error(SDL_GetError());
    }

    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!m_renderer)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        throw std::runtime_error(SDL_GetError());
    }
}

Window::~Window()
{
    if (m_renderer)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

bool Window::is_valid() const
{
    return m_window != nullptr && m_renderer != nullptr;
}

void Window::clear()
{
    SDL_SetRenderDrawColor(m_renderer, 10, 13, 27, 255);
    SDL_RenderClear(m_renderer);
}

void Window::present()
{
    SDL_RenderPresent(m_renderer);
}

void Window::draw_ui(bool rom_loaded, const std::string& rom_name)
{
    SDL_Rect frame { 280, 80, 920, 560 };
    SDL_SetRenderDrawColor(m_renderer, 24, 29, 46, 255);
    SDL_RenderFillRect(m_renderer, &frame);

    SDL_Rect left_box { 350, 150, 140, 140 };
    SDL_Rect center_box { 575, 250, 140, 140 };
    SDL_Rect right_box { 800, 350, 140, 140 };

    SDL_SetRenderDrawColor(m_renderer, 213, 120, 120, 255);
    SDL_RenderFillRect(m_renderer, &left_box);

    SDL_SetRenderDrawColor(m_renderer, 116, 148, 222, 255);
    SDL_RenderFillRect(m_renderer, &center_box);

    SDL_SetRenderDrawColor(m_renderer, 229, 193, 114, 255);
    SDL_RenderFillRect(m_renderer, &right_box);

    SDL_Rect action_bar { 450, 560, 420, 36 };

    if (rom_loaded)
    {
        SDL_SetRenderDrawColor(m_renderer, 70, 160, 90, 255);
        SDL_SetWindowTitle(m_window, ("gb-emu - " + rom_name).c_str());
    }
    else
    {
        SDL_SetRenderDrawColor(m_renderer, 70, 90, 160, 255);
        SDL_SetWindowTitle(m_window, "gb-emu - Press O or drop a ROM file");
    }

    SDL_RenderFillRect(m_renderer, &action_bar);
}

SDL_Window* Window::native_window() const
{
    return m_window;
}

SDL_Renderer* Window::native_renderer() const
{
    return m_renderer;
}
}