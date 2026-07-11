#include "cpu.h"

namespace gb {

u16 Registers::af() const { return static_cast<u16>((a << 8) | (f & 0xF0)); }
u16 Registers::bc() const { return static_cast<u16>((b << 8) | c); }
u16 Registers::de() const { return static_cast<u16>((d << 8) | e); }
u16 Registers::hl() const { return static_cast<u16>((h << 8) | l); }

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
    ime_ = false;
    ime_enable_pending_ = false;

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
    if (faulted_ || bus_ == nullptr) {
        return;
    }

    if (service_interrupts()) {
        ++instruction_count_;
        return;
    }

    if (ime_enable_pending_) {
        ime_ = true;
        ime_enable_pending_ = false;
    }

    if (halted_) {
        ++instruction_count_;
        return;
    }

    last_opcode_pc_ = registers_.pc;
    const u8 opcode = fetch8();
    last_opcode_ = opcode;
    execute(opcode);
    ++instruction_count_;
}

const Registers& Cpu::registers() const { return registers_; }
bool Cpu::is_halted() const { return halted_; }
bool Cpu::has_fault() const { return faulted_; }
u8 Cpu::last_opcode() const { return last_opcode_; }
u16 Cpu::last_opcode_pc() const { return last_opcode_pc_; }
u8 Cpu::fault_opcode() const { return fault_opcode_; }
u16 Cpu::fault_pc() const { return fault_pc_; }
std::uint64_t Cpu::instruction_count() const { return instruction_count_; }
const Bus* Cpu::bus() const { return bus_; }

u8 Cpu::fetch8() {
    const u8 value = bus_->read(registers_.pc);
    ++registers_.pc;
    return value;
}

u16 Cpu::fetch16() {
    const u8 lo = fetch8();
    const u8 hi = fetch8();
    return static_cast<u16>((hi << 8) | lo);
}

u8 Cpu::read_r8(int index) const {
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
}

void Cpu::write_r8(int index, u8 value) {
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
}

u16 Cpu::read_r16(int index) const {
    switch (index) {
        case 0: return registers_.bc();
        case 1: return registers_.de();
        case 2: return registers_.hl();
        case 3: return registers_.sp;
        default: return 0x0000;
    }
}

void Cpu::write_r16(int index, u16 value) {
    switch (index) {
        case 0: registers_.set_bc(value); break;
        case 1: registers_.set_de(value); break;
        case 2: registers_.set_hl(value); break;
        case 3: registers_.sp = value; break;
        default: break;
    }
}

u16 Cpu::read_r16stk(int index) const {
    switch (index) {
        case 0: return registers_.bc();
        case 1: return registers_.de();
        case 2: return registers_.hl();
        case 3: return registers_.af();
        default: return 0x0000;
    }
}

void Cpu::write_r16stk(int index, u16 value) {
    switch (index) {
        case 0: registers_.set_bc(value); break;
        case 1: registers_.set_de(value); break;
        case 2: registers_.set_hl(value); break;
        case 3: registers_.set_af(value); break;
        default: break;
    }
}

void Cpu::push16(u16 value) {
    registers_.sp = static_cast<u16>(registers_.sp - 1);
    bus_->write(registers_.sp, static_cast<u8>(value >> 8));
    registers_.sp = static_cast<u16>(registers_.sp - 1);
    bus_->write(registers_.sp, static_cast<u8>(value & 0xFF));
}

u16 Cpu::pop16() {
    const u8 lo = bus_->read(registers_.sp);
    registers_.sp = static_cast<u16>(registers_.sp + 1);
    const u8 hi = bus_->read(registers_.sp);
    registers_.sp = static_cast<u16>(registers_.sp + 1);
    return static_cast<u16>((hi << 8) | lo);
}

    namespace {
    constexpr u16 kInterruptVectors[5] = {
        0x40, // VBlank
        0x48, // LCD STAT
        0x50, // Timer
        0x58, // Serial
        0x60  // Joypad
    };
}

    bool Cpu::service_interrupts() {
    const u8 ie = bus_->read(0xFFFF);
    const u8 iflags = bus_->read(0xFF0F);
    const u8 pending = static_cast<u8>(ie & iflags & 0x1F);

    if (pending == 0) {
        return false;
    }

    if (halted_) {
        halted_ = false;
    }

    if (!ime_) {
        return false;
    }

    for (int bit = 0; bit < 5; ++bit) {
        const u8 mask = static_cast<u8>(1u << bit);
        if ((pending & mask) == 0) {
            continue;
        }

        ime_ = false;

        u8 new_if = static_cast<u8>(iflags & ~mask);
        bus_->write(0xFF0F, new_if);

        push16(registers_.pc);
        registers_.pc = kInterruptVectors[bit];
        return true;
    }

    return false;
}

void Cpu::fault(u8 opcode) {
    faulted_ = true;
    fault_opcode_ = opcode;
    fault_pc_ = last_opcode_pc_;
}

void Cpu::set_z(bool value) { if (value) registers_.f |= 0x80; else registers_.f &= static_cast<u8>(~0x80); }
void Cpu::set_n(bool value) { if (value) registers_.f |= 0x40; else registers_.f &= static_cast<u8>(~0x40); }
void Cpu::set_h(bool value) { if (value) registers_.f |= 0x20; else registers_.f &= static_cast<u8>(~0x20); }
void Cpu::set_c(bool value) { if (value) registers_.f |= 0x10; else registers_.f &= static_cast<u8>(~0x10); }

bool Cpu::get_z() const { return (registers_.f & 0x80) != 0; }
bool Cpu::get_n() const { return (registers_.f & 0x40) != 0; }
bool Cpu::get_h() const { return (registers_.f & 0x20) != 0; }
bool Cpu::get_c() const { return (registers_.f & 0x10) != 0; }

void Cpu::add_a(u8 value, bool with_carry) {
    const u8 carry = with_carry && get_c() ? 1 : 0;
    const u16 result = static_cast<u16>(registers_.a) + value + carry;

    set_z(static_cast<u8>(result) == 0);
    set_n(false);
    set_h(((registers_.a & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    set_c(result > 0xFF);

    registers_.a = static_cast<u8>(result);
}

void Cpu::sub_a(u8 value, bool with_carry) {
    const u8 carry = with_carry && get_c() ? 1 : 0;
    const u16 result = static_cast<u16>(registers_.a) - value - carry;

    set_z(static_cast<u8>(result) == 0);
    set_n(true);
    set_h((registers_.a & 0x0F) < ((value & 0x0F) + carry));
    set_c(static_cast<u16>(registers_.a) < static_cast<u16>(value) + carry);

    registers_.a = static_cast<u8>(result);
}

void Cpu::and_a(u8 value) {
    registers_.a &= value;
    set_z(registers_.a == 0);
    set_n(false);
    set_h(true);
    set_c(false);
}

void Cpu::or_a(u8 value) {
    registers_.a |= value;
    set_z(registers_.a == 0);
    set_n(false);
    set_h(false);
    set_c(false);
}

void Cpu::xor_a(u8 value) {
    registers_.a ^= value;
    set_z(registers_.a == 0);
    set_n(false);
    set_h(false);
    set_c(false);
}

void Cpu::cp_a(u8 value) {
    set_z(registers_.a == value);
    set_n(true);
    set_h((registers_.a & 0x0F) < (value & 0x0F));
    set_c(registers_.a < value);
}

void Cpu::inc_r8(u8& reg) {
    const u8 before = reg;
    reg = static_cast<u8>(reg + 1);
    set_z(reg == 0);
    set_n(false);
    set_h((before & 0x0F) == 0x0F);
}

void Cpu::dec_r8(u8& reg) {
    const u8 before = reg;
    reg = static_cast<u8>(reg - 1);
    set_z(reg == 0);
    set_n(true);
    set_h((before & 0x0F) == 0x00);
}

void Cpu::daa() {
    u8 correction = 0;
    bool carry = get_c();

    if (!get_n()) {
        if (get_h() || (registers_.a & 0x0F) > 0x09) {
            correction |= 0x06;
        }
        if (carry || registers_.a > 0x99) {
            correction |= 0x60;
            carry = true;
        }
        registers_.a = static_cast<u8>(registers_.a + correction);
    } else {
        if (get_h()) {
            correction |= 0x06;
        }
        if (carry) {
            correction |= 0x60;
        }
        registers_.a = static_cast<u8>(registers_.a - correction);
    }

    set_z(registers_.a == 0);
    set_h(false);
    set_c(carry);
}

    void Cpu::cpl() {
    registers_.a = static_cast<u8>(~registers_.a);
    set_n(true);
    set_h(true);
}

void Cpu::scf() {
    set_n(false);
    set_h(false);
    set_c(true);
}

void Cpu::ccf() {
    set_n(false);
    set_h(false);
    set_c(!get_c());
}

bool Cpu::check_condition(int cond) const {
    switch (cond) {
        case 0: return !get_z();
        case 1: return get_z();
        case 2: return !get_c();
        case 3: return get_c();
        default: return false;
    }
}

void Cpu::jr_relative(bool condition) {
    const int8_t offset = static_cast<int8_t>(fetch8());
    if (condition) {
        registers_.pc = static_cast<u16>(registers_.pc + offset);
    }
}

void Cpu::jp_absolute(bool condition) {
    const u16 address = fetch16();
    if (condition) {
        registers_.pc = address;
    }
}

void Cpu::call_absolute(bool condition) {
    const u16 address = fetch16();
    if (condition) {
        push16(registers_.pc);
        registers_.pc = address;
    }
}

void Cpu::ret(bool condition) {
    if (condition) {
        registers_.pc = pop16();
    }
}

void Cpu::rst(u16 address) {
    push16(registers_.pc);
    registers_.pc = address;
}

void Cpu::execute_cb(u8 opcode) {
    const int group = opcode >> 6;
    const int bit = (opcode >> 3) & 0x07;
    const int reg = opcode & 0x07;

    u8 value = read_r8(reg);

    switch (group) {
        case 0: {
            bool carry = false;
            u8 result = value;

            switch (bit) {
                case 0:
                    carry = (value & 0x80) != 0;
                    result = static_cast<u8>((value << 1) | (carry ? 1 : 0));
                    break;
                case 1:
                    carry = (value & 0x01) != 0;
                    result = static_cast<u8>((value >> 1) | (carry ? 0x80 : 0x00));
                    break;
                case 2: {
                    const u8 old_c = get_c() ? 1 : 0;
                    carry = (value & 0x80) != 0;
                    result = static_cast<u8>((value << 1) | old_c);
                    break;
                }
                case 3: {
                    const u8 old_c = get_c() ? 0x80 : 0x00;
                    carry = (value & 0x01) != 0;
                    result = static_cast<u8>((value >> 1) | old_c);
                    break;
                }
                case 4:
                    carry = (value & 0x80) != 0;
                    result = static_cast<u8>(value << 1);
                    break;
                case 5:
                    carry = (value & 0x01) != 0;
                    result = static_cast<u8>((value >> 1) | (value & 0x80));
                    break;
                case 6:
                    result = static_cast<u8>((value << 4) | (value >> 4));
                    break;
                case 7:
                    carry = (value & 0x01) != 0;
                    result = static_cast<u8>(value >> 1);
                    break;
                default:
                    fault(opcode);
                    return;
            }

            write_r8(reg, result);
            registers_.f = 0;
            set_z(result == 0);
            set_c(carry);
            return;
        }

        case 1:
            set_z((value & (1u << bit)) == 0);
            set_n(false);
            set_h(true);
            return;

        case 2:
            value &= static_cast<u8>(~(1u << bit));
            write_r8(reg, value);
            return;

        case 3:
            value |= static_cast<u8>(1u << bit);
            write_r8(reg, value);
            return;

        default:
            fault(opcode);
            return;
    }
}

void Cpu::execute(u8 opcode) {
    if (opcode >= 0x40 && opcode <= 0x7F) {
        if (opcode == 0x76) {
            halted_ = true;
            return;
        }

        const int dst = (opcode >> 3) & 0x07;
        const int src = opcode & 0x07;
        write_r8(dst, read_r8(src));
        return;
    }

    if (opcode >= 0x80 && opcode <= 0x87) {
        add_a(read_r8(opcode & 0x07), false);
        return;
    }

    if (opcode >= 0x88 && opcode <= 0x8F) {
        add_a(read_r8(opcode & 0x07), true);
        return;
    }

    if (opcode >= 0x90 && opcode <= 0x97) {
        sub_a(read_r8(opcode & 0x07), false);
        return;
    }

    if (opcode >= 0x98 && opcode <= 0x9F) {
        sub_a(read_r8(opcode & 0x07), true);
        return;
    }

    if (opcode >= 0xA0 && opcode <= 0xA7) {
        and_a(read_r8(opcode & 0x07));
        return;
    }

    if (opcode >= 0xA8 && opcode <= 0xAF) {
        xor_a(read_r8(opcode & 0x07));
        return;
    }

    if (opcode >= 0xB0 && opcode <= 0xB7) {
        or_a(read_r8(opcode & 0x07));
        return;
    }

    if (opcode >= 0xB8 && opcode <= 0xBF) {
        cp_a(read_r8(opcode & 0x07));
        return;
    }

    switch (opcode) {
        case 0x00: break;

        case 0x01:
        case 0x11:
        case 0x21:
        case 0x31:
            write_r16((opcode >> 4) & 0x03, fetch16());
            break;

        case 0x02:
            bus_->write(registers_.bc(), registers_.a);
            break;
        case 0x12:
            bus_->write(registers_.de(), registers_.a);
            break;
        case 0x0A:
            registers_.a = bus_->read(registers_.bc());
            break;
        case 0x1A:
            registers_.a = bus_->read(registers_.de());
            break;

        case 0x03:
        case 0x13:
        case 0x23:
        case 0x33: {
            const int idx = (opcode >> 4) & 0x03;
            write_r16(idx, static_cast<u16>(read_r16(idx) + 1));
            break;
        }

        case 0x0B:
        case 0x1B:
        case 0x2B:
        case 0x3B: {
            const int idx = (opcode >> 4) & 0x03;
            write_r16(idx, static_cast<u16>(read_r16(idx) - 1));
            break;
        }

        case 0x04: inc_r8(registers_.b); break;
        case 0x05: dec_r8(registers_.b); break;
        case 0x06: registers_.b = fetch8(); break;

        case 0x0C: inc_r8(registers_.c); break;
        case 0x0D: dec_r8(registers_.c); break;
        case 0x0E: registers_.c = fetch8(); break;

        case 0x14: inc_r8(registers_.d); break;
        case 0x15: dec_r8(registers_.d); break;
        case 0x16: registers_.d = fetch8(); break;

        case 0x1C: inc_r8(registers_.e); break;
        case 0x1D: dec_r8(registers_.e); break;
        case 0x1E: registers_.e = fetch8(); break;

        case 0x24: inc_r8(registers_.h); break;
        case 0x25: dec_r8(registers_.h); break;
        case 0x26: registers_.h = fetch8(); break;

        case 0x2C: inc_r8(registers_.l); break;
        case 0x2D: dec_r8(registers_.l); break;
        case 0x2E: registers_.l = fetch8(); break;

        case 0x3C: inc_r8(registers_.a); break;
        case 0x3D: dec_r8(registers_.a); break;
        case 0x3E: registers_.a = fetch8(); break;

        case 0x07: {
            const bool carry = (registers_.a & 0x80) != 0;
            registers_.a = static_cast<u8>((registers_.a << 1) | (carry ? 1 : 0));
            registers_.f = 0;
            set_c(carry);
            break;
        }

        case 0x0F: {
            const bool carry = (registers_.a & 0x01) != 0;
            registers_.a = static_cast<u8>((registers_.a >> 1) | (carry ? 0x80 : 0x00));
            registers_.f = 0;
            set_c(carry);
            break;
        }

        case 0x17: {
            const bool old_carry = get_c();
            const bool new_carry = (registers_.a & 0x80) != 0;
            registers_.a = static_cast<u8>((registers_.a << 1) | (old_carry ? 1 : 0));
            registers_.f = 0;
            set_c(new_carry);
            break;
        }

        case 0x1F: {
            const bool old_carry = get_c();
            const bool new_carry = (registers_.a & 0x01) != 0;
            registers_.a = static_cast<u8>((registers_.a >> 1) | (old_carry ? 0x80 : 0x00));
            registers_.f = 0;
            set_c(new_carry);
            break;
        }

        case 0x08: {
            const u16 address = fetch16();
            bus_->write(address, static_cast<u8>(registers_.sp & 0xFF));
            bus_->write(static_cast<u16>(address + 1), static_cast<u8>(registers_.sp >> 8));
            break;
        }

        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39: {
            const u16 hl = registers_.hl();
            const u16 value = read_r16((opcode >> 4) & 0x03);
            const u32 result = static_cast<u32>(hl) + value;
            set_n(false);
            set_h(((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF);
            set_c(result > 0xFFFF);
            registers_.set_hl(static_cast<u16>(result));
            break;
        }

        case 0x18: jr_relative(true); break;
        case 0x20: jr_relative(!get_z()); break;
        case 0x28: jr_relative(get_z()); break;
        case 0x30: jr_relative(!get_c()); break;
        case 0x38: jr_relative(get_c()); break;

        case 0x22:
            bus_->write(registers_.hl(), registers_.a);
            registers_.set_hl(static_cast<u16>(registers_.hl() + 1));
            break;
        case 0x2A:
            registers_.a = bus_->read(registers_.hl());
            registers_.set_hl(static_cast<u16>(registers_.hl() + 1));
            break;
        case 0x32:
            bus_->write(registers_.hl(), registers_.a);
            registers_.set_hl(static_cast<u16>(registers_.hl() - 1));
            break;
        case 0x3A:
            registers_.a = bus_->read(registers_.hl());
            registers_.set_hl(static_cast<u16>(registers_.hl() - 1));
            break;

        case 0x27: daa(); break;
        case 0x2F: cpl(); break;
        case 0x37: scf(); break;
        case 0x3F: ccf(); break;

        case 0x34: {
            u8 value = bus_->read(registers_.hl());
            inc_r8(value);
            bus_->write(registers_.hl(), value);
            break;
        }

        case 0x35: {
            u8 value = bus_->read(registers_.hl());
            dec_r8(value);
            bus_->write(registers_.hl(), value);
            break;
        }

        case 0x36:
            bus_->write(registers_.hl(), fetch8());
            break;

        case 0xC0: ret(!get_z()); break;
        case 0xC8: ret(get_z()); break;
        case 0xD0: ret(!get_c()); break;
        case 0xD8: ret(get_c()); break;
        case 0xC9: ret(true); break;
        case 0xD9:
            registers_.pc = pop16();
            ime_ = true;
            break;

        case 0xC2: jp_absolute(!get_z()); break;
        case 0xCA: jp_absolute(get_z()); break;
        case 0xD2: jp_absolute(!get_c()); break;
        case 0xDA: jp_absolute(get_c()); break;
        case 0xC3: jp_absolute(true); break;
        case 0xE9:
            registers_.pc = registers_.hl();
            break;

        case 0xC4: call_absolute(!get_z()); break;
        case 0xCC: call_absolute(get_z()); break;
        case 0xD4: call_absolute(!get_c()); break;
        case 0xDC: call_absolute(get_c()); break;
        case 0xCD: call_absolute(true); break;

        case 0xC1:
        case 0xD1:
        case 0xE1:
        case 0xF1:
            write_r16stk((opcode >> 4) & 0x03, pop16());
            break;

        case 0xC5:
        case 0xD5:
        case 0xE5:
        case 0xF5:
            push16(read_r16stk((opcode >> 4) & 0x03));
            break;

        case 0xC6: add_a(fetch8(), false); break;
        case 0xCE: add_a(fetch8(), true); break;
        case 0xD6: sub_a(fetch8(), false); break;
        case 0xDE: sub_a(fetch8(), true); break;
        case 0xE6: and_a(fetch8()); break;
        case 0xEE: xor_a(fetch8()); break;
        case 0xF6: or_a(fetch8()); break;
        case 0xFE: cp_a(fetch8()); break;

        case 0xC7: rst(0x00); break;
        case 0xCF: rst(0x08); break;
        case 0xD7: rst(0x10); break;
        case 0xDF: rst(0x18); break;
        case 0xE7: rst(0x20); break;
        case 0xEF: rst(0x28); break;
        case 0xF7: rst(0x30); break;
        case 0xFF: rst(0x38); break;

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

        case 0xF2:
            registers_.a = bus_->read(static_cast<u16>(0xFF00 + registers_.c));
            break;

        case 0xFA: {
            const u16 address = fetch16();
            registers_.a = bus_->read(address);
            break;
        }

        case 0xE8: {
            const int8_t imm = static_cast<int8_t>(fetch8());
            const u16 sp = registers_.sp;
            const u16 result = static_cast<u16>(sp + imm);
            set_z(false);
            set_n(false);
            set_h(((sp & 0x0F) + (static_cast<u16>(imm) & 0x0F)) > 0x0F);
            set_c(((sp & 0xFF) + (static_cast<u16>(imm) & 0xFF)) > 0xFF);
            registers_.sp = result;
            break;
        }

        case 0xF8: {
            const int8_t imm = static_cast<int8_t>(fetch8());
            const u16 sp = registers_.sp;
            const u16 result = static_cast<u16>(sp + imm);
            set_z(false);
            set_n(false);
            set_h(((sp & 0x0F) + (static_cast<u16>(imm) & 0x0F)) > 0x0F);
            set_c(((sp & 0xFF) + (static_cast<u16>(imm) & 0xFF)) > 0xFF);
            registers_.set_hl(result);
            break;
        }

        case 0xF9:
            registers_.sp = registers_.hl();
            break;

        case 0xF3:
            ime_ = false;
            ime_enable_pending_ = false;
            break;

        case 0xFB:
            ime_enable_pending_ = true;
            break;

        case 0xCB:
            execute_cb(fetch8());
            break;

        case 0x10:
            (void)fetch8();
            break;

        case 0xD3:
        case 0xDB:
        case 0xDD:
        case 0xE3:
        case 0xE4:
        case 0xEB:
        case 0xEC:
        case 0xED:
        case 0xF4:
        case 0xFC:
        case 0xFD:
            fault(opcode);
            break;

        default:
            fault(opcode);
            break;
    }
}

}