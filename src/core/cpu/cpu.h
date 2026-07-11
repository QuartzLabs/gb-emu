#pragma once

#include <cstdint>

#include "registers.h"
#include "../bus/bus.h"

namespace gb {

class Cpu {
public:
    void connect_bus(Bus* bus);
    void reset();
    void step();

    [[nodiscard]] const Registers& registers() const;
    [[nodiscard]] bool is_halted() const;
    [[nodiscard]] bool has_fault() const;
    [[nodiscard]] u8 last_opcode() const;
    [[nodiscard]] u16 last_opcode_pc() const;
    [[nodiscard]] u8 fault_opcode() const;
    [[nodiscard]] u16 fault_pc() const;
    [[nodiscard]] std::uint64_t instruction_count() const;
    [[nodiscard]] const Bus* bus() const;

private:
    [[nodiscard]] u8 fetch8();
    [[nodiscard]] u16 fetch16();
    [[nodiscard]] u8 read_r8(int index) const;
    void write_r8(int index, u8 value);

    [[nodiscard]] u16 read_r16(int index) const;
    void write_r16(int index, u16 value);
    [[nodiscard]] u16 read_r16stk(int index) const;
    void write_r16stk(int index, u16 value);

    void push16(u16 value);
    [[nodiscard]] u16 pop16();

    void execute(u8 opcode);
    void execute_cb(u8 opcode);
    void fault(u8 opcode);

    void set_z(bool value);
    void set_n(bool value);
    void set_h(bool value);
    void set_c(bool value);

    [[nodiscard]] bool get_z() const;
    [[nodiscard]] bool get_n() const;
    [[nodiscard]] bool get_h() const;
    [[nodiscard]] bool get_c() const;

    void add_a(u8 value, bool with_carry = false);
    void sub_a(u8 value, bool with_carry = false);
    void and_a(u8 value);
    void or_a(u8 value);
    void xor_a(u8 value);
    void cp_a(u8 value);

    void inc_r8(u8& reg);
    void dec_r8(u8& reg);

    void daa();
    void cpl();
    void scf();
    void ccf();

    [[nodiscard]] bool check_condition(int cond) const;
    void jr_relative(bool condition);
    void jp_absolute(bool condition);
    void call_absolute(bool condition);
    void ret(bool condition);
    void rst(u16 address);

    Bus* bus_ = nullptr;
    Registers registers_{};

    bool halted_ = false;
    bool faulted_ = false;
    bool ime_ = false;
    bool ime_enable_pending_ = false;
    bool service_interrupts();

    u8 last_opcode_ = 0x00;
    u16 last_opcode_pc_ = 0x0000;
    u8 fault_opcode_ = 0x00;
    u16 fault_pc_ = 0x0000;
    std::uint64_t instruction_count_ = 0;
};

}