#include "bus.h"

#include <algorithm>

namespace gb {

void Bus::attach_cartridge(Cartridge* cartridge) {
    cartridge_ = cartridge;
}

void Bus::reset() {
    std::ranges::fill(vram_, 0);
    std::ranges::fill(wram_, 0);
    std::ranges::fill(oam_, 0);
    std::ranges::fill(io_, 0);
    std::ranges::fill(hram_, 0);

    interrupt_enable_ = 0;
    interrupt_flag_ = 0xE1;
    div_counter_ = 0;
    tima_counter_ = 0;
    joypad_select_ = 0x30;
    joypad_state_ = 0x0F;

    io_[0x00] = 0xCF;
    io_[0x04] = 0x00;
    io_[0x05] = 0x00;
    io_[0x06] = 0x00;
    io_[0x07] = 0x00;
    io_[0x0F] = interrupt_flag_;
}

void Bus::tick(int cycles) {
    update_timer(cycles);
}

void Bus::request_interrupt(u8 bit) {
    interrupt_flag_ = static_cast<u8>(interrupt_flag_ | (1u << bit));
    io_[0x0F] = interrupt_flag_;
}

void Bus::update_timer(int cycles) {
    div_counter_ = static_cast<u16>(div_counter_ + cycles);
    io_[0x04] = static_cast<u8>(div_counter_ >> 8);

    const u8 tac = io_[0x07];
    if ((tac & 0x04) == 0) {
        return;
    }

    static constexpr int timer_thresholds[4] = {1024, 16, 64, 256};
    const int threshold = timer_thresholds[tac & 0x03];

    tima_counter_ += static_cast<u16>(cycles);
    while (tima_counter_ >= threshold) {
        tima_counter_ = static_cast<u16>(tima_counter_ - threshold);
        if (io_[0x05] == 0xFF) {
            io_[0x05] = io_[0x06];
            request_interrupt(2);
        } else {
            ++io_[0x05];
        }
    }
}

u8 Bus::read_joypad() const {
    u8 value = static_cast<u8>(joypad_select_ | 0x0F);

    if ((joypad_select_ & 0x10) == 0) {
        value = static_cast<u8>((joypad_select_ & 0xF0) | (joypad_state_ & 0x0F));
    } else if ((joypad_select_ & 0x20) == 0) {
        value = static_cast<u8>((joypad_select_ & 0xF0) | (joypad_state_ >> 4));
    }

    return value;
}

void Bus::write_joypad(u8 value) {
    joypad_select_ = static_cast<u8>(value & 0x30);
    io_[0x00] = static_cast<u8>((value & 0x30) | 0xCF);
}

u8 Bus::read(u16 address) const {
    if (address <= 0x7FFF) {
        return cartridge_ ? cartridge_->read(address) : 0xFF;
    }

    if (address >= 0x8000 && address <= 0x9FFF) {
        return vram_[address - 0x8000];
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
        return cartridge_ ? cartridge_->read(address) : 0xFF;
    }

    if (address >= 0xC000 && address <= 0xDFFF) {
        return wram_[address - 0xC000];
    }

    if (address >= 0xE000 && address <= 0xFDFF) {
        return wram_[address - 0xE000];
    }

    if (address >= 0xFE00 && address <= 0xFE9F) {
        return oam_[address - 0xFE00];
    }

    if (address >= 0xFEA0 && address <= 0xFEFF) {
        return 0xFF;
    }

    if (address == 0xFF00) {
        return read_joypad();
    }

    if (address == 0xFF0F) {
        return static_cast<u8>(interrupt_flag_ | 0xE0);
    }

    if (address == 0xFFFF) {
        return interrupt_enable_;
    }

    if (address >= 0xFF00 && address <= 0xFF7F) {
        return io_[address - 0xFF00];
    }

    if (address >= 0xFF80 && address <= 0xFFFE) {
        return hram_[address - 0xFF80];
    }

    return 0xFF;
}

void Bus::write(u16 address, u8 value) {
    if (address <= 0x7FFF) {
        if (cartridge_) {
            cartridge_->write(address, value);
        }
        return;
    }

    if (address >= 0x8000 && address <= 0x9FFF) {
        vram_[address - 0x8000] = value;
        return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
        if (cartridge_) {
            cartridge_->write(address, value);
        }
        return;
    }

    if (address >= 0xC000 && address <= 0xDFFF) {
        wram_[address - 0xC000] = value;
        return;
    }

    if (address >= 0xE000 && address <= 0xFDFF) {
        wram_[address - 0xE000] = value;
        return;
    }

    if (address >= 0xFE00 && address <= 0xFE9F) {
        oam_[address - 0xFE00] = value;
        return;
    }

    if (address >= 0xFEA0 && address <= 0xFEFF) {
        return;
    }

    if (address == 0xFF00) {
        write_joypad(value);
        return;
    }

    if (address == 0xFF04) {
        div_counter_ = 0;
        io_[0x04] = 0;
        return;
    }

    if (address == 0xFF05) {
        io_[0x05] = value;
        return;
    }

    if (address == 0xFF06) {
        io_[0x06] = value;
        return;
    }

    if (address == 0xFF07) {
        io_[0x07] = static_cast<u8>(value & 0x07);
        return;
    }

    if (address == 0xFF0F) {
        interrupt_flag_ = static_cast<u8>(value | 0xE0);
        io_[0x0F] = interrupt_flag_;
        return;
    }

    if (address == 0xFFFF) {
        interrupt_enable_ = value;
        return;
    }

    if (address >= 0xFF00 && address <= 0xFF7F) {
        io_[address - 0xFF00] = value;
        return;
    }

    if (address >= 0xFF80 && address <= 0xFFFE) {
        hram_[address - 0xFF80] = value;
        return;
    }
}

} // namespace gb