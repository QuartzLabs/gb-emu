#pragma once

#include "../../common/types.h"

namespace gb {

    struct Registers {
        u8 a = 0x01;
        u8 f = 0xB0;
        u8 b = 0x00;
        u8 c = 0x13;
        u8 d = 0x00;
        u8 e = 0xD8;
        u8 h = 0x01;
        u8 l = 0x4D;
        u16 sp = 0xFFFE;
        u16 pc = 0x0100;

        [[nodiscard]] u16 af() const;
        [[nodiscard]] u16 bc() const;
        [[nodiscard]] u16 de() const;
        [[nodiscard]] u16 hl() const;

        void set_af(u16 value);
        void set_bc(u16 value);
        void set_de(u16 value);
        void set_hl(u16 value);
    };

}