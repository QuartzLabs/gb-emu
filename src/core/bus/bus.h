#pragma once

#include <array>

#include "../../common/types.h"
#include "../cartridge/cartridge.h"

namespace gb {

    class Bus {
    public:
        void attach_cartridge(Cartridge* cartridge);
        void reset();
        void tick(int cycles);

        [[nodiscard]] u8 read(u16 address) const;
        void write(u16 address, u8 value);

    private:
        void update_timer(int cycles);
        void request_interrupt(u8 bit);
        [[nodiscard]] u8 read_joypad() const;
        void write_joypad(u8 value);

        Cartridge* cartridge_ = nullptr;

        std::array<u8, 0x2000> vram_{};
        std::array<u8, 0x2000> wram_{};
        std::array<u8, 0x00A0> oam_{};
        std::array<u8, 0x0080> io_{};
        std::array<u8, 0x007F> hram_{};

        u8 interrupt_enable_ = 0;
        u8 interrupt_flag_ = 0xE1;

        u16 div_counter_ = 0;
        u16 tima_counter_ = 0;
        u8 joypad_select_ = 0x30;
        u8 joypad_state_ = 0x0F;
    };

}