// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <lib/log.hpp>
#include <stdint.h>

namespace kernel::drivers::net::nicmgr {

enum type_t
{
    RTL8139,
    E1000
};

class NetCard
{
    public:
    char name[32] = "New network card";
    uint8_t MAC[6];
    uint8_t IPv4[4];
    type_t type;
    uint64_t uniqueid;

    virtual void send(uint8_t *data, uint64_t length)
    {
        error("Send function for this device is not available!");
    }
};

extern bool initialised;
extern vector<NetCard*> cards;

void init();
}