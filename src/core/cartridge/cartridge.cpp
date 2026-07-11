#include "cartridge.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace gb {

namespace {

bool cartridge_has_battery(u8 type) {
    switch (type) {
        case 0x03:
        case 0x06:
        case 0x09:
        case 0x0D:
        case 0x0F:
        case 0x10:
        case 0x13:
        case 0x1B:
        case 0x1E:
        case 0x22:
        case 0xFF:
            return true;
        default:
            return false;
    }
}

bool cartridge_has_ram(u8 type) {
    switch (type) {
        case 0x02:
        case 0x03:
        case 0x08:
        case 0x09:
        case 0x0C:
        case 0x0D:
        case 0x10:
        case 0x12:
        case 0x13:
        case 0x16:
        case 0x17:
        case 0x1A:
        case 0x1B:
        case 0x1D:
        case 0x1E:
        case 0x22:
        case 0xFC:
            return true;
        default:
            return false;
    }
}

std::size_t decode_ram_size(u8 code) {
    switch (code) {
        case 0x00: return 0;
        case 0x01: return 2 * 1024;
        case 0x02: return 8 * 1024;
        case 0x03: return 32 * 1024;
        case 0x04: return 128 * 1024;
        case 0x05: return 64 * 1024;
        default:   return 0;
    }
}

} // namespace

bool Cartridge::load_from_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    rom_ = std::vector<u8>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );

    if (rom_.empty()) {
        return false;
    }

    path_ = path;
    parse_header();
    setup_banking();
    reset();
    return true;
}

void Cartridge::reset() {
    ram_enabled_ = false;
    rom_bank_low5_ = 1;
    bank_high2_ = 0;
    banking_mode_ = 0;
}

bool Cartridge::is_loaded() const {
    return !rom_.empty();
}

u8 Cartridge::read(u16 address) const {
    if (!is_loaded()) {
        return 0xFF;
    }

    if (address <= 0x3FFF) {
        return read_rom_bank0(address);
    }

    if (address >= 0x4000 && address <= 0x7FFF) {
        return read_rom_banked(address);
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
        return read_ram(address);
    }

    return 0xFF;
}

void Cartridge::write(u16 address, u8 value) {
    if (!is_loaded()) {
        return;
    }

    if (!is_mbc1()) {
        if (address >= 0xA000 && address <= 0xBFFF && !ram_.empty()) {
            ram_[address - 0xA000] = value;
        }
        return;
    }

    if (address <= 0x1FFF) {
        ram_enabled_ = (value & 0x0F) == 0x0A;
        return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
        rom_bank_low5_ = value & 0x1F;
        if (rom_bank_low5_ == 0) {
            rom_bank_low5_ = 1;
        }
        return;
    }

    if (address >= 0x4000 && address <= 0x5FFF) {
        bank_high2_ = value & 0x03;
        return;
    }

    if (address >= 0x6000 && address <= 0x7FFF) {
        banking_mode_ = value & 0x01;
        return;
    }

    if (address >= 0xA000 && address <= 0xBFFF && !ram_.empty() && ram_enabled_) {
        std::size_t ram_bank = 0;
        if (banking_mode_ == 1 && ram_bank_count() > 1) {
            ram_bank = bank_high2_ % ram_bank_count();
        }

        const std::size_t offset = ram_bank * 0x2000 + (address - 0xA000);
        if (offset < ram_.size()) {
            ram_[offset] = value;
        }
    }
}

const CartridgeHeader& Cartridge::header() const {
    return header_;
}

const std::filesystem::path& Cartridge::path() const {
    return path_;
}

std::size_t Cartridge::size() const {
    return rom_.size();
}

bool Cartridge::is_mbc1() const {
    switch (header_.cartridge_type) {
        case 0x01:
        case 0x02:
        case 0x03:
            return true;
        default:
            return false;
    }
}

bool Cartridge::has_ram() const {
    return !ram_.empty();
}

void Cartridge::parse_header() {
    header_ = {};

    if (rom_.size() < 0x150) {
        return;
    }

    std::string title;
    for (std::size_t i = 0x0134; i <= 0x0143; ++i) {
        const char value = static_cast<char>(rom_[i]);
        if (value == '\0') {
            break;
        }
        title.push_back(value);
    }

    title.erase(
        std::find_if(title.rbegin(), title.rend(), [](unsigned char ch) {
            return ch != ' ' && ch != '\0';
        }).base(),
        title.end()
    );

    header_.title = title;
    header_.cartridge_type = rom_[0x0147];
    header_.rom_size = rom_[0x0148];
    header_.ram_size = rom_[0x0149];
    header_.has_battery = cartridge_has_battery(header_.cartridge_type);
}

void Cartridge::setup_banking() {
    const std::size_t ram_size_bytes = decode_ram_size(header_.ram_size);
    ram_.assign(ram_size_bytes, 0);
}

std::size_t Cartridge::rom_bank_count() const {
    if (rom_.empty()) {
        return 0;
    }
    return rom_.size() / 0x4000;
}

std::size_t Cartridge::ram_bank_count() const {
    if (ram_.empty()) {
        return 0;
    }
    return std::max<std::size_t>(1, ram_.size() / 0x2000);
}

u8 Cartridge::read_rom_bank0(u16 address) const {
    std::size_t bank = 0;

    if (is_mbc1() && banking_mode_ == 1) {
        bank = (static_cast<std::size_t>(bank_high2_) << 5) % std::max<std::size_t>(1, rom_bank_count());
    }

    const std::size_t offset = bank * 0x4000 + address;
    if (offset >= rom_.size()) {
        return 0xFF;
    }

    return rom_[offset];
}

u8 Cartridge::read_rom_banked(u16 address) const {
    std::size_t bank = rom_bank_low5_;

    if (is_mbc1()) {
        bank |= static_cast<std::size_t>(bank_high2_) << 5;
    }

    const std::size_t total_banks = std::max<std::size_t>(1, rom_bank_count());
    bank %= total_banks;

    if (bank == 0) {
        bank = 1;
    }

    const std::size_t offset = bank * 0x4000 + (address - 0x4000);
    if (offset >= rom_.size()) {
        return 0xFF;
    }

    return rom_[offset];
}

u8 Cartridge::read_ram(u16 address) const {
    if (ram_.empty() || !ram_enabled_) {
        return 0xFF;
    }

    std::size_t bank = 0;
    if (is_mbc1() && banking_mode_ == 1 && ram_bank_count() > 1) {
        bank = bank_high2_ % ram_bank_count();
    }

    const std::size_t offset = bank * 0x2000 + (address - 0xA000);
    if (offset >= ram_.size()) {
        return 0xFF;
    }

    return ram_[offset];
}

}