#include "cpu.h"

namespace gb {

u16 Registers::af() const {
    return static_cast<u16>((a << 8) | (f & 0xF0));
}

u16 Registers::bc() const {
    return static_cast<u16>((b << 8) | c);
}

u16 Registers::de() const {
    return static_cast<u16>((d << 8) | e);
}

u16 Registers::hl() const {
    return static_cast<u16>((h << 8) | l);
}

void Registers::set_af(u16 value) {
    a = static_cast<u8>(value >> 8);
    f = static_cast<u8>(value & 0xF0);
}

void Registers::set_bc(u16 value) {
    b = static_cast<u8>(value >> 8);
    c = static_cast<u8>(value & 0xFF);
}

void Registers::set_de(u16 value) {
    d = static_cast<u8>(value >> 8);
    e = static_cast<u8>(value & 0xFF);
}

void Registers::set_hl(u16 value) {
    h = static_cast<u8>(value >> 8);
    l = static_cast<u8>(value & 0xFF);
}

void Cpu::connect_bus(Bus* bus) {
    bus_ = bus;
}

void Cpu::reset() {
    registers_ = {};
    halted_ = false;
}

void Cpu::step() {
    if (halted_ || bus_ == nullptr) {
        return;
    }

    const u8 opcode = fetch8();
    execute(opcode);
}

const Registers& Cpu::registers() const {
    return registers_;
}

bool Cpu::is_halted() const {
    return halted_;
}

u8 Cpu::fetch8() {
    const u8 value = bus_->read(registers_.pc);
    ++registers_.pc;
    return value;
}

u16 Cpu::fetch16() {
    const u8 low = fetch8();
    const u8 high = fetch8();
    return static_cast<u16>((high << 8) | low);
}

void Cpu::execute(u8 opcode) {
    switch (opcode) {
        case 0x00:
            break;

        case 0x3E:
            registers_.a = fetch8();
            break;

        case 0x06:
            registers_.b = fetch8();
            break;

        case 0x0E:
            registers_.c = fetch8();
            break;

        case 0x16:
            registers_.d = fetch8();
            break;

        case 0x1E:
            registers_.e = fetch8();
            break;

        case 0x26:
            registers_.h = fetch8();
            break;

        case 0x2E:
            registers_.l = fetch8();
            break;

        case 0x21:
            registers_.set_hl(fetch16());
            break;

        case 0x31:
            registers_.sp = fetch16();
            break;

        case 0x32:
            bus_->write(registers_.hl(), registers_.a);
            registers_.set_hl(static_cast<u16>(registers_.hl() - 1));
            break;

        case 0xAF:
            registers_.a ^= registers_.a;
            registers_.f = 0x80;
            break;

        case 0xC3:
            registers_.pc = fetch16();
            break;

        case 0x76:
            halted_ = true;
            break;

        default:
            halted_ = true;
            break;
    }
}

}