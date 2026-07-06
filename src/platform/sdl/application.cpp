#include "application.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <stdexcept>

namespace gb::platform::sdl
{
namespace
{
std::string to_lower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

    return value;
}
}

Application::Application()
    : m_running(false)
    , m_initialized(false)
    , m_rom_loaded(false)
    , m_rom_name("No ROM loaded")
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

int Application::run()
{
    if (!initialize())
    {
        return 1;
    }

    m_running = true;

    while (m_running)
    {
        handle_events();
        render();
    }

    return 0;
}

bool Application::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "SDL initialization error",
            SDL_GetError(),
            nullptr);
        return false;
    }

    try
    {
        m_window = new Window("gb-emu", 1280, 720);
    }
    catch (const std::exception&)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Window creation error",
            SDL_GetError(),
            nullptr);
        SDL_Quit();
        return false;
    }

    m_initialized = true;
    return true;
}

void Application::shutdown()
{
    if (m_window)
    {
        delete m_window;
        m_window = nullptr;
    }

    if (m_initialized)
    {
        SDL_Quit();
        m_initialized = false;
    }
}

void Application::handle_events()
{
    SDL_Event event {};

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            m_running = false;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                m_running = false;
            }
            else if (event.key.keysym.sym == SDLK_o)
            {
                try_open_rom_dialog();
            }
            break;

        case SDL_DROPFILE:
        {
            if (event.drop.file)
            {
                std::string dropped_path = event.drop.file;
                load_rom(dropped_path);
                SDL_free(event.drop.file);
            }
            break;
        }

        default:
            break;
        }
    }
}

void Application::render()
{
    m_window->clear();
    m_window->draw_ui(m_rom_loaded, m_rom_name);
    m_window->present();
}

bool Application::try_open_rom_dialog()
{
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_INFORMATION,
        "Open ROM",
        "Native file dialog is temporarily disabled in this build.\n\nFor now, drag and drop a .gb or .gbc file onto the window.",
        m_window ? m_window->native_window() : nullptr);

    return false;
}

bool Application::load_rom(const std::string& path)
{
    if (!is_supported_rom(path))
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Unsupported ROM",
            "Please select a .gb or .gbc file.",
            m_window ? m_window->native_window() : nullptr);
        return false;
    }

    if (!m_cartridge.load_from_file(path))
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "ROM load error",
            "Failed to load the selected ROM file.",
            m_window ? m_window->native_window() : nullptr);
        return false;
    }

    m_rom_loaded = true;
    m_rom_path = path;
    m_rom_name = m_cartridge.path().filename().string();

    reset_emulator_with_loaded_rom();

    return true;
}

bool Application::is_supported_rom(const std::string& path) const
{
    const std::string ext = to_lower(std::filesystem::path(path).extension().string());
    return ext == ".gb" || ext == ".gbc";
}

void Application::reset_emulator_with_loaded_rom()
{
    m_bus.attach_cartridge(&m_cartridge);
    m_bus.reset();

    m_cpu.connect_bus(&m_bus);
    m_cpu.reset();
}
}