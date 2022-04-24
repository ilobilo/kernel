// Copyright (C) 2021-2022  ilobilo

#pragma once

enum keys
{
    CAPSLOCK = 0x3A,
    NUMLOCK = 0x45,
    SCROLLLOCK = 0x46,

    L_SHIFT_DOWN = 0x2A,
    L_SHIFT_UP = 0xAA,

    R_SHIFT_DOWN = 0x36,
    R_SHIFT_UP = 0xB6,

    CTRL_DOWN = 0x1D,
    CTRL_UP = 0x9D,

    ALT_DOWN = 0x38,
    ALT_UP = 0xB8,

    HOME = 0x47,
    END = 0x4F,

    PGUP = 0x49,
    PGDN = 0x51,

    INSERT = 0x52,
    DELETE = 0x53,

    UP = 0x48,
    DOWN = 0x50,
    LEFT = 0x4B,
    RIGHT = 0x4D,

    KPD_ENTER = 0x1C,
    KPD_SLASH = 0x35
};

extern unsigned char kbdus[];
extern unsigned char kbdus_shft[];
extern unsigned char kbdus_caps[];
extern unsigned char kbdus_capsshft[];

extern unsigned char kbdus_numpad[];