#include <filesystem>
#include <iomanip>
#include <iostream>

#include "core/bus/bus.h"
#include "core/cartridge/cartridge.h"
#include "core/cpu/cpu.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: gb-emu <rom-file>\n";
        return 1;
    }

    const std::filesystem::path rom_path = argv[1];

    gb::Cartridge cartridge;
    if (!cartridge.load_from_file(rom_path)) {
        std::cerr << "Failed to load ROM: " << rom_path << '\n';
        return 1;
    }

    gb::Bus bus;
    bus.attach_cartridge(&cartridge);
    bus.reset();

    gb::Cpu cpu;
    cpu.connect_bus(&bus);
    cpu.reset();

    std::cout << "Loaded ROM: " << cartridge.path().filename().string() << '\n';
    std::cout << "Title: " << cartridge.header().title << '\n';
    std::cout << "ROM size: " << cartridge.size() << " bytes\n";

    for (int i = 0; i < 32 && !cpu.is_halted(); ++i) {
        const auto& before = cpu.registers();
        std::cout << "PC=0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << before.pc << '\n';
        cpu.step();
        const auto& after = cpu.registers();
        std::cout
            << "AF=0x" << std::setw(4) << after.af()
            << " BC=0x" << std::setw(4) << after.bc()
            << " DE=0x" << std::setw(4) << after.de()
            << " HL=0x" << std::setw(4) << after.hl()
            << " SP=0x" << std::setw(4) << after.sp
            << " PC=0x" << std::setw(4) << after.pc
            << '\n';
    }

    return 0;
}