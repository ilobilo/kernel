// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/math.hpp>
#include <stdint.h>

namespace kernel::drivers::ps2 {

static constexpr uint64_t PS2_MAX_TIMEOUT = 1000;
static constexpr uint64_t PS2_MAX_RESENDS = 10;
static constexpr uint8_t PS2_KBD_SCANCODE = 2;

static constexpr uint64_t KBD_BUFFSIZE = 1024;

static constexpr uint8_t PS2_X_SIGN = 0b00010000;
static constexpr uint8_t PS2_Y_SIGN = 0b00100000;
static constexpr uint8_t PS2_X_OVER = 0b01000000;
static constexpr uint8_t PS2_Y_OVER = 0b10000000;

enum mousestate
{
    PS2_NONE = 0b00000000,
    PS2_LEFT = 0b00000001,
    PS2_MIDDLE = 0b00000100,
    PS2_RIGHT = 0b00000010,
};

enum devices
{
    PS2_DEVICE_NONE = 0,
    PS2_DEVICE_FIRST = 1,
    PS2_DEVICE_SECOND = 2
};

enum ioports
{
    PS2_PORT_DATA = 0x60,
    PS2_PORT_STATUS = 0x64,
    PS2_PORT_COMMAND = 0x64,
};

enum status
{
    PS2_OUTPUT_FULL = (1 << 0),
    PS2_INPUT_FULL = (1 << 1),
    PS2_MOUSE_BYTE = (1 << 5)
};

enum mousetype
{
    PS2_MOUSE_NORMAL = 0,
    PS2_MOUSE_SCROLL = 3,
    PS2_MOUSE_5_BUTTON = 4
};

enum cmds
{
    PS2_DISABLE_FIRST = 0xAD,
    PS2_ENABLE_FIRST = 0xAE,
    PS2_TEST_FIRST = 0xAB,
    PS2_DISABLE_SECOND = 0xA7,
    PS2_ENABLE_SECOND = 0xA8,
    PS2_WRITE_SECOND = 0xD4,
    PS2_TEST_SECOND = 0xA9,
    PS2_TEST_CONTROLER = 0xAA,
    PS2_READ_CONFIG = 0x20,
    PS2_WRITE_CONFIG = 0x60
};

enum config
{
    PS2_FIRST_IRQ_MASK = (1 << 0),
    PS2_SECOND_IRQ_MASK = (1 << 1),
    PS2_SECOND_CLOCK = (1 << 5),
    PS2_TRANSLATION = (1 << 6)
};

enum kbdcmd
{
    PS2_KEYBOARD_REPEAT = 0xF3,
    PS2_KEYBOARD_SET_LEDS = 0xED,
    PS2_KEYBOARD_SCANCODE_SET = 0xF0
};

enum mousecmd
{
    PS2_MOUSE_DEFAULTS = 0xF6,
    PS2_MOUSE_SAMPLE_RATE = 0xF3,
    PS2_MOUSE_READ = 0xEB,
    PS2_MOUSE_RESOLUTION = 0xE8
};

enum cmd
{
    PS2_DEVICE_RESET = 0xFF,
    PS2_DEVICE_DISABLE = 0xF5,
    PS2_DEVICE_ENABLE = 0xF4,
    PS2_DEVICE_IDENTIFY = 0xF2
};

struct kbd_mod_t
{
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;
    bool numlock : 1;
    bool capslock : 1;
    bool scrolllock : 1;
};

extern kbd_mod_t kbd_mod;

extern bool initialised;
extern bool kbdinitialised;
extern bool mouseinitialised;

extern bool mousevmware;

extern point mousepos;
extern point mouseposold;
extern uint32_t mousebordercol;
extern uint32_t mouseinsidecol;

mousestate getmousestate();

void mousedraw();
void mouseclear();

char getchar();
[[clang::optnone]] char *getline();

void init();
}