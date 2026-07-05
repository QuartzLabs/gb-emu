#pragma once

#include <array>

#include "../../common/types.h"
#include "../cartridge/cartridge.h"

namespace gb {

    class Bus {
    public:
        void attach_cartridge(const Cartridge* cartridge);
        void reset();

        [[nodiscard]] u8 read(u16 address) const;
        void write(u16 address, u8 value);

    private:
        const Cartridge* cartridge_ = nullptr;
        std::array<u8, 0x2000> vram_{};
        std::array<u8, 0x2000> wram_{};
        std::array<u8, 0xA0> oam_{};
        std::array<u8, 0x80> hram_{};
        u8 interrupt_enable_ = 0;
    };

}