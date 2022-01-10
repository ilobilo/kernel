// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <lib/log.hpp>
#include <stdint.h>

namespace kernel::drivers::net::nicmgr {

enum type_t
{
    RTL8139,
    RTL8169,
    E1000
};

class NIC
{
    public:
    char name[32] = "New NIC";
    uint8_t MAC[6];
    uint8_t IPv4[4];
    type_t type;
    uint64_t uniqueid;
    bool debug = NET_DEBUG;

    virtual void send(uint8_t *data, uint64_t length)
    {
        error("Send function for this device is not available!");
    }
};

extern bool initialised;
extern vector<NIC*> nics;

void init();
}