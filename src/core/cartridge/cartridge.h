#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "../../common/types.h"

namespace gb {

    struct CartridgeHeader {
        std::string title;
        u8 cartridge_type = 0;
        u8 rom_size = 0;
        u8 ram_size = 0;
        bool has_battery = false;
    };

    class Cartridge {
    public:
        bool load_from_file(const std::filesystem::path& path);
        void reset();

        [[nodiscard]] bool is_loaded() const;
        [[nodiscard]] u8 read(u16 address) const;
        void write(u16 address, u8 value);

        [[nodiscard]] const CartridgeHeader& header() const;
        [[nodiscard]] const std::filesystem::path& path() const;
        [[nodiscard]] std::size_t size() const;

        [[nodiscard]] bool is_mbc1() const;
        [[nodiscard]] bool has_ram() const;

    private:
        void parse_header();
        void setup_banking();
        [[nodiscard]] std::size_t rom_bank_count() const;
        [[nodiscard]] std::size_t ram_bank_count() const;
        [[nodiscard]] u8 read_rom_bank0(u16 address) const;
        [[nodiscard]] u8 read_rom_banked(u16 address) const;
        [[nodiscard]] u8 read_ram(u16 address) const;

        std::filesystem::path path_;
        std::vector<u8> rom_;
        std::vector<u8> ram_;
        CartridgeHeader header_{};

        bool ram_enabled_ = false;
        u8 rom_bank_low5_ = 1;
        u8 bank_high2_ = 0;
        u8 banking_mode_ = 0;
    };

}