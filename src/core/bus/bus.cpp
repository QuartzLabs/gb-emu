#include "bus.h"

#include <algorithm>

namespace gb {

    void Bus::attach_cartridge(const Cartridge* cartridge) {
        cartridge_ = cartridge;
    }

    void Bus::reset() {
        std::ranges::fill(vram_, 0);
        std::ranges::fill(wram_, 0);
        std::ranges::fill(oam_, 0);
        std::ranges::fill(hram_, 0);
        interrupt_enable_ = 0;
    }

    u8 Bus::read(u16 address) const {
        if (address <= 0x7FFF) {
            return cartridge_ ? cartridge_->read(address) : 0xFF;
        }

        if (address >= 0x8000 && address <= 0x9FFF) {
            return vram_[address - 0x8000];
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

        if (address >= 0xFF80 && address <= 0xFFFE) {
            return hram_[address - 0xFF80];
        }

        if (address == 0xFFFF) {
            return interrupt_enable_;
        }

        return 0x00;
    }

    void Bus::write(u16 address, u8 value) {
        if (address >= 0x8000 && address <= 0x9FFF) {
            vram_[address - 0x8000] = value;
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

        if (address >= 0xFF80 && address <= 0xFFFE) {
            hram_[address - 0xFF80] = value;
            return;
        }

        if (address == 0xFFFF) {
            interrupt_enable_ = value;
        }
    }

}