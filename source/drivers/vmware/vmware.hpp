// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/ps2/ps2.hpp>
#include <stdint.h>
#include <stddef.h>

using namespace kernel::drivers::ps2;

namespace kernel::drivers::vmware {

static constexpr uint32_t VMWARE_MAGIC = 0x564D5868;
static constexpr uint16_t VMWARE_PORT = 0x5658;

static constexpr uint16_t CMD_GETVERSION = 10;
static constexpr uint16_t CMD_ABSPOINTER_DATA = 39;
static constexpr uint16_t CMD_ABSPOINTER_STATUS = 40;
static constexpr uint16_t CMD_ABSPOINTER_COMMAND = 41;

static constexpr uint32_t ABSPOINTER_ENABLE = 0x45414552;
static constexpr uint32_t ABSPOINTER_RELATIVE = 0xF5;
static constexpr uint32_t ABSPOINTER_ABSOLUTE = 0x53424152;

struct CMD
{
    union {
        uint32_t ax;
        uint32_t magic;
    };
    union {
        uint32_t bx;
        size_t size;
    };
    union {
        uint32_t cx;
        uint16_t command;
    };
    union {
        uint32_t dx;
        uint16_t port;
    };
    uint32_t si;
    uint32_t di;
};

extern bool initialised;

void vmware_send(CMD *cmd);
bool is_vmware_backdoor();

void handle_mouse();

ps2::mousestate getmousestate();

void mouse_absolute();
void mouse_relative();

void init();
}