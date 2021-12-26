// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/ps2/mouse/mouse.hpp>
#include <stdint.h>
#include <stddef.h>

using namespace kernel::drivers::ps2;

namespace kernel::drivers::vmware {

#define VMWARE_MAGIC  0x564D5868
#define VMWARE_PORT   0x5658
#define VMWARE_PORTHB 0x5659

#define CMD_GETVERSION         10
#define CMD_ABSPOINTER_DATA    39
#define CMD_ABSPOINTER_STATUS  40
#define CMD_ABSPOINTER_COMMAND 41

#define ABSPOINTER_ENABLE   0x45414552
#define ABSPOINTER_RELATIVE 0xF5
#define ABSPOINTER_ABSOLUTE 0x53424152

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

mouse::mousestate getmousestate();

void mouse_absolute();
void mouse_relative();

void init();
}