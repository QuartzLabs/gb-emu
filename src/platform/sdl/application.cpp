#include "application.h"

#include <filesystem>
#include <memory>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <commdlg.h>
#endif

namespace gb::platform::sdl {
    namespace
    {
        constexpr int kWindowWidth = 1280;
        constexpr int kWindowHeight = 720;
        constexpr int kStepsPerFrame = 2000;

        std::string rom_name_from_path(const std::filesystem::path& path)
        {
            return path.filename().string();
        }

#ifdef _WIN32
        std::string wstring_to_utf8(const std::wstring& value)
        {
            if (value.empty())
            {
                return {};
            }

            const int size_needed = WideCharToMultiByte(
                CP_UTF8,
                0,
                value.c_str(),
                static_cast<int>(value.size()),
                nullptr,
                0,
                nullptr,
                nullptr
            );

            if (size_needed <= 0)
            {
                return {};
            }

            std::string result(size_needed, '\0');

            WideCharToMultiByte(
                CP_UTF8,
                0,
                value.c_str(),
                static_cast<int>(value.size()),
                result.data(),
                size_needed,
                nullptr,
                nullptr
            );

            return result;
        }
#endif
    }

    Application::Application()
        : m_running(false)
        , m_initialized(false)
        , m_rom_loaded(false)
        , m_rom_name()
        , m_rom_path()
        , m_cartridge()
        , m_bus()
        , m_cpu()
        , m_window(nullptr)
    {
    }

    Application::~Application()
    {
        shutdown();
    }

    bool Application::initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        try
        {
            m_window = std::make_unique<Window>("gb-emu", kWindowWidth, kWindowHeight);
        }
        catch (...)
        {
            m_window.reset();
            return false;
        }

        if (!m_window || !m_window->is_valid())
        {
            m_window.reset();
            return false;
        }

        m_initialized = true;
        m_running = true;
        return true;
    }

    void Application::shutdown()
    {
        m_running = false;
        m_window.reset();
        m_initialized = false;
    }

    bool Application::is_supported_rom(const std::string& path) const
    {
        const std::filesystem::path rom_path(path);
        const std::string extension = rom_path.extension().string();

        return extension == ".gb" || extension == ".gbc";
    }

    bool Application::try_open_rom_dialog()
    {
#ifdef _WIN32
        std::wstring file_buffer(MAX_PATH, L'\0');

        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = file_buffer.data();
        ofn.nMaxFile = static_cast<DWORD>(file_buffer.size());
        ofn.lpstrFilter = L"Game Boy ROMs\0*.gb;*.gbc\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        if (!GetOpenFileNameW(&ofn))
        {
            return false;
        }

        return load_rom(wstring_to_utf8(ofn.lpstrFile));
#else
        return false;
#endif
    }

    bool Application::load_rom(const std::string& path)
    {
        if (!is_supported_rom(path))
        {
            return false;
        }

        if (!m_cartridge.load_from_file(path))
        {
            return false;
        }

        m_rom_path = path;
        m_rom_name = rom_name_from_path(std::filesystem::path(path));
        m_rom_loaded = true;

        reset_emulator_with_loaded_rom();
        return true;
    }

    void Application::reset_emulator_with_loaded_rom()
    {
        if (!m_rom_loaded)
        {
            return;
        }

        m_bus.reset();
        m_bus.attach_cartridge(&m_cartridge);
        m_cpu.connect_bus(&m_bus);
        m_cpu.reset();
    }

    void Application::run_emulator_frame()
    {
        if (!m_rom_loaded)
        {
            return;
        }

        constexpr int kInstructionsPerFrame = 2000;
        constexpr int kApproxCyclesPerInstruction = 4;

        for (int i = 0; i < kInstructionsPerFrame; ++i)
        {
            const bool was_halted = m_cpu.is_halted();

            m_cpu.step();
            m_bus.tick(kApproxCyclesPerInstruction);

            if (m_cpu.has_fault())
            {
                m_running = false;
                break;
            }

            if (was_halted && m_cpu.is_halted())
            {
                continue;
            }
        }
    }

    void Application::handle_events()
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    m_running = false;
                    break;

                case SDL_DROPFILE:
                    if (event.drop.file != nullptr)
                    {
                        load_rom(event.drop.file);
                        SDL_free(event.drop.file);
                    }
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        m_running = false;
                    }
#ifdef _WIN32
                    else if (event.key.keysym.sym == SDLK_o &&
                             (event.key.keysym.mod & KMOD_CTRL))
                    {
                        try_open_rom_dialog();
                    }
#endif
                    break;

                default:
                    break;
            }
        }
    }

    void Application::render()
    {
        if (!m_window)
        {
            return;
        }

        m_window->clear();
        m_window->draw_ui(
            m_rom_loaded,
            m_rom_name,
            m_cpu
        );
        m_window->present();
    }

    int Application::run()
    {
        if (!initialize())
        {
            return 1;
        }

        while (m_running)
        {
            handle_events();
            run_emulator_frame();
            render();
        }

        shutdown();
        return 0;
    }
}