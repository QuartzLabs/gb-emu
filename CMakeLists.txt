cmake_minimum_required(VERSION 3.20)

project(gb-emu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)

set(SOURCES
        src/main.cpp
        src/platform/sdl/application.cpp
        src/platform/sdl/window.cpp
        src/core/bus/bus.cpp
        src/core/cartridge/cartridge.cpp
        src/core/cpu/cpu.cpp
)

add_executable(gb-emu
        ${SOURCES}
)

target_include_directories(gb-emu PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(gb-emu PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)