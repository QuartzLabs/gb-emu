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
    faulted_ = false;
    last_opcode_ = 0x00;
    last_opcode_pc_ = 0x0000;
    fault_opcode_ = 0x00;
    fault_pc_ = 0x0000;
    instruction_count_ = 0;

    registers_.pc = 0x0100;
    registers_.sp = 0xFFFE;
    registers_.a = 0x01;
    registers_.f = 0xB0;
    registers_.b = 0x00;
    registers_.c = 0x13;
    registers_.d = 0x00;
    registers_.e = 0xD8;
    registers_.h = 0x01;
    registers_.l = 0x4D;
}

void Cpu::step() {
    if (halted_ || faulted_ || bus_ == nullptr) {
        return;
    }

    last_opcode_pc_ = registers_.pc;
    const u8 opcode = fetch8();
    last_opcode_ = opcode;
    execute(opcode);
    ++instruction_count_;
}

const Registers& Cpu::registers() const {
    return registers_;
}

bool Cpu::is_halted() const {
    return halted_;
}

bool Cpu::has_fault() const {
    return faulted_;
}

u8 Cpu::last_opcode() const {
    return last_opcode_;
}

u16 Cpu::last_opcode_pc() const {
    return last_opcode_pc_;
}

u8 Cpu::fault_opcode() const {
    return fault_opcode_;
}

u16 Cpu::fault_pc() const {
    return fault_pc_;
}

std::uint64_t Cpu::instruction_count() const {
    return instruction_count_;
}

const Bus* Cpu::bus() const {
    return bus_;
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

void Cpu::execute_cb(u8 opcode) {
    auto read_r8 = [&](int index) -> u8 {
        switch (index) {
        case 0: return registers_.b;
        case 1: return registers_.c;
        case 2: return registers_.d;
        case 3: return registers_.e;
        case 4: return registers_.h;
        case 5: return registers_.l;
        case 6: return bus_->read(registers_.hl());
        case 7: return registers_.a;
        default: return 0x00;
        }
    };

    auto write_r8 = [&](int index, u8 value) {
        switch (index) {
        case 0: registers_.b = value; break;
        case 1: registers_.c = value; break;
        case 2: registers_.d = value; break;
        case 3: registers_.e = value; break;
        case 4: registers_.h = value; break;
        case 5: registers_.l = value; break;
        case 6: bus_->write(registers_.hl(), value); break;
        case 7: registers_.a = value; break;
        default: break;
        }
    };

    const int group = opcode >> 6;
    const int bit = (opcode >> 3) & 0x07;
    const int reg = opcode & 0x07;

    switch (group) {
    case 0: {
        u8 value = read_r8(reg);
        u8 result = value;
        bool carry = false;

        switch ((opcode >> 3) & 0x07) {
        case 0:
            carry = (value & 0x80) != 0;
            result = static_cast<u8>((value << 1) | (carry ? 1 : 0));
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;

        case 1:
            carry = (value & 0x01) != 0;
            result = static_cast<u8>((value >> 1) | (carry ? 0x80 : 0x00));
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;

        case 2: {
            const u8 old_carry = (registers_.f & 0x10) ? 1 : 0;
            carry = (value & 0x80) != 0;
            result = static_cast<u8>((value << 1) | old_carry);
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;
        }

        case 3: {
            const u8 old_carry = (registers_.f & 0x10) ? 0x80 : 0x00;
            carry = (value & 0x01) != 0;
            result = static_cast<u8>((value >> 1) | old_carry);
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;
        }

        case 4:
            carry = (value & 0x80) != 0;
            result = static_cast<u8>(value << 1);
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;

        case 5:
            carry = (value & 0x01) != 0;
            result = static_cast<u8>((value >> 1) | (value & 0x80));
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;

        case 6:
            result = static_cast<u8>((value << 4) | (value >> 4));
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            break;

        case 7:
            carry = (value & 0x01) != 0;
            result = static_cast<u8>(value >> 1);
            registers_.f = 0;
            if (result == 0) registers_.f |= 0x80;
            if (carry) registers_.f |= 0x10;
            break;

        default:
            faulted_ = true;
            fault_opcode_ = opcode;
            fault_pc_ = last_opcode_pc_;
            return;
        }

        write_r8(reg, result);
        break;
    }

    case 1: {
        const u8 value = read_r8(reg);
        const u8 result = static_cast<u8>(value & (1u << bit));
        registers_.f &= 0x10;
        registers_.f |= 0x20;
        if (result == 0) {
            registers_.f |= 0x80;
        }
        break;
    }

    case 2: {
        u8 value = read_r8(reg);
        value &= static_cast<u8>(~(1u << bit));
        write_r8(reg, value);
        break;
    }

    case 3: {
        u8 value = read_r8(reg);
        value |= static_cast<u8>(1u << bit);
        write_r8(reg, value);
        break;
    }

    default:
        faulted_ = true;
        fault_opcode_ = opcode;
        fault_pc_ = last_opcode_pc_;
        break;
    }
}

void Cpu::execute(u8 opcode) {
    switch (opcode) {
    case 0x00:
        break;

    case 0x01:
        registers_.set_bc(fetch16());
        break;

    case 0x05:
        --registers_.b;
        break;

    case 0x06:
        registers_.b = fetch8();
        break;

    case 0x0C:
        ++registers_.c;
        break;

    case 0x0D:
        --registers_.c;
        break;

    case 0x0E:
        registers_.c = fetch8();
        break;

    case 0x11:
        registers_.set_de(fetch16());
        break;

    case 0x13:
        registers_.set_de(static_cast<u16>(registers_.de() + 1));
        break;

    case 0x16:
        registers_.d = fetch8();
        break;

    case 0x17: {
        const u8 old_carry = (registers_.f & 0x10) ? 1 : 0;
        const u8 carry = (registers_.a & 0x80) ? 0x10 : 0x00;
        registers_.a = static_cast<u8>((registers_.a << 1) | old_carry);
        registers_.f = carry;
        if (registers_.a == 0) registers_.f |= 0x80;
        break;
    }

    case 0x18: {
        const auto offset = static_cast<signed char>(fetch8());
        registers_.pc = static_cast<u16>(registers_.pc + offset);
        break;
    }

    case 0x1A:
        registers_.a = bus_->read(registers_.de());
        break;

    case 0x1E:
        registers_.e = fetch8();
        break;

    case 0x20: {
        const auto offset = static_cast<signed char>(fetch8());
        if ((registers_.f & 0x80) == 0) {
            registers_.pc = static_cast<u16>(registers_.pc + offset);
        }
        break;
    }

    case 0x21:
        registers_.set_hl(fetch16());
        break;

    case 0x22:
        bus_->write(registers_.hl(), registers_.a);
        registers_.set_hl(static_cast<u16>(registers_.hl() + 1));
        break;

    case 0x23:
        registers_.set_hl(static_cast<u16>(registers_.hl() + 1));
        break;

    case 0x26:
        registers_.h = fetch8();
        break;

    case 0x2E:
        registers_.l = fetch8();
        break;

    case 0x31:
        registers_.sp = fetch16();
        break;

    case 0x32:
        bus_->write(registers_.hl(), registers_.a);
        registers_.set_hl(static_cast<u16>(registers_.hl() - 1));
        break;

    case 0x3E:
        registers_.a = fetch8();
        break;

    case 0x4F:
        registers_.c = registers_.a;
        break;

    case 0x57:
        registers_.d = registers_.a;
        break;

    case 0x67:
        registers_.h = registers_.a;
        break;

    case 0x77:
        bus_->write(registers_.hl(), registers_.a);
        break;

    case 0x78:
        registers_.a = registers_.b;
        break;

    case 0x79:
        registers_.a = registers_.c;
        break;

    case 0x7A:
        registers_.a = registers_.d;
        break;

    case 0x7B:
        registers_.a = registers_.e;
        break;

    case 0x7C:
        registers_.a = registers_.h;
        break;

    case 0x7D:
        registers_.a = registers_.l;
        break;

    case 0x7E:
        registers_.a = bus_->read(registers_.hl());
        break;

    case 0xAF:
        registers_.a ^= registers_.a;
        registers_.f = 0x80;
        break;

    case 0xC1: {
        const u8 low = bus_->read(registers_.sp);
        registers_.sp = static_cast<u16>(registers_.sp + 1);
        const u8 high = bus_->read(registers_.sp);
        registers_.sp = static_cast<u16>(registers_.sp + 1);
        registers_.set_bc(static_cast<u16>((high << 8) | low));
        break;
    }

    case 0xC3:
        registers_.pc = fetch16();
        break;

    case 0xC5:
        registers_.sp = static_cast<u16>(registers_.sp - 1);
        bus_->write(registers_.sp, static_cast<u8>(registers_.bc() >> 8));
        registers_.sp = static_cast<u16>(registers_.sp - 1);
        bus_->write(registers_.sp, static_cast<u8>(registers_.bc() & 0xFF));
        break;

    case 0xC9: {
        const u8 low = bus_->read(registers_.sp);
        registers_.sp = static_cast<u16>(registers_.sp + 1);
        const u8 high = bus_->read(registers_.sp);
        registers_.sp = static_cast<u16>(registers_.sp + 1);
        registers_.pc = static_cast<u16>((high << 8) | low);
        break;
    }

    case 0xCD: {
        const u16 address = fetch16();
        const u16 return_address = registers_.pc;
        registers_.sp = static_cast<u16>(registers_.sp - 1);
        bus_->write(registers_.sp, static_cast<u8>(return_address >> 8));
        registers_.sp = static_cast<u16>(registers_.sp - 1);
        bus_->write(registers_.sp, static_cast<u8>(return_address & 0xFF));
        registers_.pc = address;
        break;
    }

    case 0xE0: {
        const u8 offset = fetch8();
        bus_->write(static_cast<u16>(0xFF00 + offset), registers_.a);
        break;
    }

    case 0xE2:
        bus_->write(static_cast<u16>(0xFF00 + registers_.c), registers_.a);
        break;

    case 0xEA: {
        const u16 address = fetch16();
        bus_->write(address, registers_.a);
        break;
    }

    case 0xF0: {
        const u8 offset = fetch8();
        registers_.a = bus_->read(static_cast<u16>(0xFF00 + offset));
        break;
    }

    case 0xF3:
        break;

    case 0xFE: {
        const u8 value = fetch8();
        const u8 result = static_cast<u8>(registers_.a - value);
        registers_.f = 0x40;
        if (result == 0) {
            registers_.f |= 0x80;
        }
        if ((registers_.a & 0x0F) < (value & 0x0F)) {
            registers_.f |= 0x20;
        }
        if (registers_.a < value) {
            registers_.f |= 0x10;
        }
        break;
    }

    case 0xCB:
        execute_cb(fetch8());
        break;

    case 0x76:
        halted_ = true;
        break;

    default:
        faulted_ = true;
        fault_opcode_ = opcode;
        fault_pc_ = last_opcode_pc_;
        break;
    }
}

}