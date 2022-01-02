// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <stdint.h>

namespace kernel::drivers::net::stack::ethernet {

enum protocol_t
{
    TYPE_ARP = 0x0806,
    TYPE_IPv4 = 0x0800
};

struct [[gnu::packed]] ethHdr
{
    uint8_t dmac[6];
    uint8_t smac[6];
    uint16_t type;
    uint8_t data[];
};

extern bool debug;

void send(nicmgr::NIC *nic, uint8_t *dmac, uint8_t *data, size_t length, uint16_t protocol);
void receive(nicmgr::NIC *nic, ethHdr *packet, size_t length);
}