// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>

#define NUM_KEYS 256
#define MAX_NUM_FUNC 256
#define MAX_NUM_KEYMAPS 256

#define KEY(t, v) (((t) << 8) | (v))
#define KEY_TYPE(x) ((x) >> 8)
#define KEY_VALUE(x) ((x) & 0xff)

enum keygs
{
    KG_SHIFT = 0,
    KG_CTRL = 2,
    KG_ALT = 3,
    KG_ALTGR = 1,
    KG_SHIFTL = 4,
    KG_KANASHIFT = 4,
    KG_SHIFTR = 5,
    KG_CTRLL = 6,
    KG_CTRLR = 7
};

enum key_types
{
    KEY_TYPE_LATIN = 0,
    KEY_TYPE_FN = 1,
    KEY_TYPE_SPEC = 2,
    KEY_TYPE_PAD = 3,
    KEY_TYPE_DEAD = 4,
    KEY_TYPE_CONS = 5,
    KEY_TYPE_CUR = 6,
    KEY_TYPE_SHIFT = 7,
    KEY_TYPE_META = 8,
    KEY_TYPE_ASCII = 9,
    KEY_TYPE_LOCK = 10,
    KEY_TYPE_LETTER = 11,
    KEY_TYPE_SLOCK = 12,
    KEY_TYPE_DEAD2 = 13,
    KEY_TYPE_BRL = 14
};

enum keys
{
    KEY_SHIFT = KEY(KEY_TYPE_SHIFT, KG_SHIFT),
    KEY_CTRL = KEY(KEY_TYPE_SHIFT, KG_CTRL),
    KEY_ALT = KEY(KEY_TYPE_SHIFT, KG_ALT),
    KEY_ALTGR = KEY(KEY_TYPE_SHIFT, KG_ALTGR),
    KEY_SHIFTL = KEY(KEY_TYPE_SHIFT, KG_SHIFTL),
    KEY_SHIFTR = KEY(KEY_TYPE_SHIFT, KG_SHIFTR),
    KEY_CTRLL = KEY(KEY_TYPE_SHIFT, KG_CTRLL),
    KEY_CTRLR = KEY(KEY_TYPE_SHIFT, KG_CTRLR),

    KEY_P0 = KEY(KEY_TYPE_PAD, 0),
    KEY_P1 = KEY(KEY_TYPE_PAD, 1),
    KEY_P2 = KEY(KEY_TYPE_PAD, 2),
    KEY_P3 = KEY(KEY_TYPE_PAD, 3),
    KEY_P4 = KEY(KEY_TYPE_PAD, 4),
    KEY_P5 = KEY(KEY_TYPE_PAD, 5),
    KEY_P6 = KEY(KEY_TYPE_PAD, 6),
    KEY_P7 = KEY(KEY_TYPE_PAD, 7),
    KEY_P8 = KEY(KEY_TYPE_PAD, 8),
    KEY_P9 = KEY(KEY_TYPE_PAD, 9),

    KEY_PCOMMA = KEY(KEY_TYPE_PAD, 15),
    KEY_PDOT = KEY(KEY_TYPE_PAD, 16),

    KEY_FIND = KEY(KEY_TYPE_FN, 20),
    KEY_INSERT = KEY(KEY_TYPE_FN, 21),
    KEY_REMOVE = KEY(KEY_TYPE_FN, 22),
    KEY_SELECT = KEY(KEY_TYPE_FN, 23),
    KEY_PGUP = KEY(KEY_TYPE_FN, 24),
    KEY_PGDN = KEY(KEY_TYPE_FN, 25),

    KEY_DOWN = KEY(KEY_TYPE_CUR, 0),
    KEY_LEFT = KEY(KEY_TYPE_CUR, 1),
    KEY_RIGHT = KEY(KEY_TYPE_CUR, 2),
    KEY_UP = KEY(KEY_TYPE_CUR, 3)
};

extern uint16_t plain_map[NUM_KEYS];
extern uint16_t shift_map[NUM_KEYS];
extern uint16_t altgr_map[NUM_KEYS];
extern uint16_t ctrl_map[NUM_KEYS];
extern uint16_t shift_ctrl_map[NUM_KEYS];
extern uint16_t alt_map[NUM_KEYS];
extern uint16_t ctrl_alt_map[NUM_KEYS];

extern uint16_t *key_maps[MAX_NUM_KEYMAPS];

extern char *func_table[MAX_NUM_FUNC];