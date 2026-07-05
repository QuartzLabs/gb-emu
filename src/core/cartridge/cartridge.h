#pragma once

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
    };

    class Cartridge {
    public:
        bool load_from_file(const std::filesystem::path& path);

        [[nodiscard]] bool is_loaded() const;
        [[nodiscard]] u8 read(u16 address) const;
        [[nodiscard]] const CartridgeHeader& header() const;
        [[nodiscard]] const std::filesystem::path& path() const;
        [[nodiscard]] std::size_t size() const;

    private:
        void parse_header();

        std::filesystem::path path_;
        std::vector<u8> rom_;
        CartridgeHeader header_{};
    };

}