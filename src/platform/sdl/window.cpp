#include "window.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace gb::platform::sdl {
    namespace
    {
        constexpr SDL_Color kColorBackground { 10, 13, 27, 255 };
        constexpr SDL_Color kColorPanel { 24, 29, 46, 255 };
        constexpr SDL_Color kColorPanelAlt { 32, 38, 60, 255 };
        constexpr SDL_Color kColorText { 230, 235, 245, 255 };
        constexpr SDL_Color kColorMuted { 150, 160, 185, 255 };
        constexpr SDL_Color kColorGreen { 70, 190, 110, 255 };
        constexpr SDL_Color kColorRed { 210, 90, 90, 255 };
        constexpr SDL_Color kColorYellow { 235, 196, 92, 255 };

        constexpr const char* kFontCandidates[] = {
            "C:/Windows/Fonts/consola.ttf",
            "C:/Windows/Fonts/lucon.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };
    }

    Window::Window(const char* title, int width, int height)
    : m_window(nullptr)
    , m_renderer(nullptr)
    , m_font(nullptr)
    , m_width(width)
    , m_height(height)
    {
        if (TTF_Init() != 0)
        {
            throw std::runtime_error(TTF_GetError());
        }

        m_window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN
        );

        if (!m_window)
        {
            TTF_Quit();
            throw std::runtime_error(SDL_GetError());
        }

        m_renderer = SDL_CreateRenderer(
            m_window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if (!m_renderer)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            TTF_Quit();
            throw std::runtime_error(SDL_GetError());
        }

        for (const char* font_path : kFontCandidates)
        {
            m_font = TTF_OpenFont(font_path, 20);
            if (m_font != nullptr)
            {
                break;
            }
        }

        if (!m_font)
        {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            TTF_Quit();
            throw std::runtime_error("Failed to load a debug font");
        }
    }

    Window::~Window()
    {
        if (m_font)
        {
            TTF_CloseFont(m_font);
            m_font = nullptr;
        }

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

        TTF_Quit();
    }

    bool Window::is_valid() const
    {
        return m_window != nullptr && m_renderer != nullptr && m_font != nullptr;
    }

    void Window::clear()
    {
        SDL_SetRenderDrawColor(
            m_renderer,
            kColorBackground.r,
            kColorBackground.g,
            kColorBackground.b,
            kColorBackground.a
        );
        SDL_RenderClear(m_renderer);
    }

    void Window::present()
    {
        SDL_RenderPresent(m_renderer);
    }

    void Window::draw_text(const std::string& text, int x, int y, SDL_Color color) const
    {
        SDL_Surface* surface = TTF_RenderUTF8_Blended(m_font, text.c_str(), color);
        if (!surface)
        {
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
        if (!texture)
        {
            SDL_FreeSurface(surface);
            return;
        }

        SDL_Rect dst { x, y, surface->w, surface->h };
        SDL_FreeSurface(surface);

        SDL_RenderCopy(m_renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    std::string Window::hex8(unsigned int value) const
    {
        std::ostringstream stream;
        stream << "0x"
               << std::uppercase
               << std::setfill('0')
               << std::setw(2)
               << std::hex
               << (value & 0xFF);
        return stream.str();
    }

    std::string Window::hex16(unsigned int value) const
    {
        std::ostringstream stream;
        stream << "0x"
               << std::uppercase
               << std::setfill('0')
               << std::setw(4)
               << std::hex
               << (value & 0xFFFF);
        return stream.str();
    }

    std::string Window::hex64(std::uint64_t value) const
    {
        std::ostringstream stream;
        stream << "0x"
               << std::uppercase
               << std::setfill('0')
               << std::setw(10)
               << std::hex
               << value;
        return stream.str();
    }

    std::string Window::memory_row(const gb::Bus* bus, unsigned int start, unsigned int pc) const
    {
        std::ostringstream stream;
        stream << std::uppercase
               << std::setfill('0')
               << std::setw(4)
               << std::hex
               << (start & 0xFFFF)
               << ": ";

        for (unsigned int i = 0; i < 8; ++i)
        {
            const unsigned int address = (start + i) & 0xFFFF;
            const unsigned int value = bus ? bus->read(static_cast<gb::u16>(address)) : 0xFF;

            if (address == (pc & 0xFFFF))
            {
                stream << '['
                       << std::setw(2)
                       << (value & 0xFF)
                       << ']';
            }
            else
            {
                stream << ' '
                       << std::setw(2)
                       << (value & 0xFF)
                       << ' ';
            }

            if (i != 7)
            {
                stream << ' ';
            }
        }

        return stream.str();
    }

    void Window::draw_ui(
        bool rom_loaded,
        const std::string& rom_name,
        const gb::Cpu& cpu
    )
    {
        const gb::Registers& registers = cpu.registers();

        if (rom_loaded)
        {
            SDL_SetWindowTitle(m_window, ("gb-emu - " + rom_name).c_str());
        }
        else
        {
            SDL_SetWindowTitle(m_window, "gb-emu - Press Ctrl+O or drop a ROM file");
        }

        SDL_Rect outer_panel { 32, 24, m_width - 64, m_height - 48 };
        SDL_SetRenderDrawColor(
            m_renderer,
            kColorPanel.r,
            kColorPanel.g,
            kColorPanel.b,
            kColorPanel.a
        );
        SDL_RenderFillRect(m_renderer, &outer_panel);

        SDL_Rect header_panel { 56, 48, m_width - 112, 72 };
        SDL_SetRenderDrawColor(
            m_renderer,
            kColorPanelAlt.r,
            kColorPanelAlt.g,
            kColorPanelAlt.b,
            kColorPanelAlt.a
        );
        SDL_RenderFillRect(m_renderer, &header_panel);

        SDL_Rect left_panel { 56, 144, 360, 520 };
        SDL_Rect middle_panel { 436, 144, 360, 520 };
        SDL_Rect right_panel { 816, 144, 392, 520 };

        SDL_RenderFillRect(m_renderer, &left_panel);
        SDL_RenderFillRect(m_renderer, &middle_panel);
        SDL_RenderFillRect(m_renderer, &right_panel);

        draw_text("GB-EMU DEBUG", 76, 70, kColorText);
        draw_text(rom_loaded ? "ROM: " + rom_name : "ROM: none loaded", 76, 96, kColorMuted);

        draw_text("CPU STATUS", 80, 168, kColorText);
        draw_text(cpu.is_halted() ? "HALT: YES" : "HALT: NO", 80, 206, cpu.is_halted() ? kColorRed : kColorGreen);
        draw_text(cpu.has_fault() ? "FAULT: YES" : "FAULT: NO", 80, 242, cpu.has_fault() ? kColorYellow : kColorGreen);

        draw_text("LAST OPCODE " + hex8(cpu.last_opcode()), 80, 292, kColorText);
        draw_text("LAST PC " + hex16(cpu.last_opcode_pc()), 80, 326, kColorText);
        draw_text("FAULT OPCODE " + hex8(cpu.fault_opcode()), 80, 376, kColorText);
        draw_text("FAULT PC " + hex16(cpu.fault_pc()), 80, 410, kColorText);
        draw_text("INST COUNT " + hex64(cpu.instruction_count()), 80, 460, kColorText);

        draw_text("REGISTERS", 460, 168, kColorText);
        draw_text("PC " + hex16(registers.pc), 460, 206, kColorText);
        draw_text("SP " + hex16(registers.sp), 460, 240, kColorText);
        draw_text("AF " + hex16(registers.af()), 460, 290, kColorText);
        draw_text("BC " + hex16(registers.bc()), 460, 324, kColorText);
        draw_text("DE " + hex16(registers.de()), 460, 358, kColorText);
        draw_text("HL " + hex16(registers.hl()), 460, 392, kColorText);
        draw_text("A " + hex8(registers.a), 460, 442, kColorText);
        draw_text("B " + hex8(registers.b), 460, 476, kColorText);
        draw_text("C " + hex8(registers.c), 460, 510, kColorText);
        draw_text("D " + hex8(registers.d), 460, 544, kColorText);
        draw_text("E " + hex8(registers.e), 460, 578, kColorText);
        draw_text("H " + hex8(registers.h), 620, 442, kColorText);
        draw_text("L " + hex8(registers.l), 620, 476, kColorText);
        draw_text("F " + hex8(registers.f), 620, 510, kColorText);

        draw_text("MEMORY AROUND PC", 840, 168, kColorText);

        const gb::Bus* bus = cpu.bus();
        const unsigned int pc = registers.pc;
        const unsigned int row0 = (pc >= 8) ? ((pc - 8) & 0xFFF8) : 0;
        const unsigned int row1 = (row0 + 8) & 0xFFFF;
        const unsigned int row2 = (row1 + 8) & 0xFFFF;
        const unsigned int row3 = (row2 + 8) & 0xFFFF;

        draw_text(memory_row(bus, row0, pc), 840, 214, kColorText);
        draw_text(memory_row(bus, row1, pc), 840, 248, kColorText);
        draw_text(memory_row(bus, row2, pc), 840, 282, kColorText);
        draw_text(memory_row(bus, row3, pc), 840, 316, kColorText);

        draw_text("HELP", 840, 390, kColorText);
        draw_text("Ctrl+O -> Open ROM", 840, 426, kColorMuted);
        draw_text("Esc -> Exit", 840, 460, kColorMuted);
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