#pragma once

#include "core/bus/bus.h"
#include "core/cartridge/cartridge.h"
#include "core/cpu/cpu.h"
#include "platform/sdl/window.h"

#include <memory>
#include <string>

namespace gb::platform::sdl
{
    class Application
    {
    public:
        Application();
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        int run();

    private:
        bool initialize();
        void shutdown();
        void handle_events();
        void render();
        void run_emulator_frame();

        bool try_open_rom_dialog();
        bool load_rom(const std::string& path);
        bool is_supported_rom(const std::string& path) const;
        void reset_emulator_with_loaded_rom();

    private:
        bool m_running;
        bool m_initialized;
        bool m_rom_loaded;
        std::string m_rom_name;
        std::string m_rom_path;

        gb::Cartridge m_cartridge;
        gb::Bus m_bus;
        gb::Cpu m_cpu;

        std::unique_ptr<Window> m_window;
    };
}