#include "cartridge.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace gb {

    bool Cartridge::load_from_file(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }

        rom_ = std::vector<u8>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        if (rom_.empty()) {
            return false;
        }

        path_ = path;
        parse_header();
        return true;
    }

    bool Cartridge::is_loaded() const {
        return !rom_.empty();
    }

    u8 Cartridge::read(u16 address) const {
        if (address >= rom_.size()) {
            return 0xFF;
        }

        return rom_[address];
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

    void Cartridge::parse_header() {
        if (rom_.size() < 0x150) {
            header_ = {};
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

        title.erase(std::find_if(title.rbegin(), title.rend(), [](unsigned char ch) {
            return ch != ' ' && ch != '\0';
        }).base(), title.end());

        header_.title = title;
        header_.cartridge_type = rom_[0x0147];
        header_.rom_size = rom_[0x0148];
        header_.ram_size = rom_[0x0149];
    }

}