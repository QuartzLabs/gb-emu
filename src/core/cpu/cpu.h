#pragma once

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

    private:
        [[nodiscard]] u8 fetch8();
        [[nodiscard]] u16 fetch16();
        void execute(u8 opcode);

        Bus* bus_ = nullptr;
        Registers registers_{};
        bool halted_ = false;
    };

}