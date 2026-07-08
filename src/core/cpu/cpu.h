#pragma once

#include "registers.h"
#include "../bus/bus.h"

#include <cstdint>

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
        void execute(u8 opcode);
        void execute_cb(u8 opcode);

        Bus* bus_ = nullptr;
        Registers registers_{};
        bool halted_ = false;
        bool faulted_ = false;
        u8 last_opcode_ = 0x00;
        u16 last_opcode_pc_ = 0x0000;
        u8 fault_opcode_ = 0x00;
        u16 fault_pc_ = 0x0000;
        std::uint64_t instruction_count_ = 0;
    };

}