cmake_minimum_required(VERSION 3.20)
project(gb_emu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(gb-emu
        src/main.cpp
        src/core/cartridge/cartridge.cpp
        src/core/bus/bus.cpp
        src/core/cpu/cpu.cpp
)

target_include_directories(gb-emu PRIVATE src)