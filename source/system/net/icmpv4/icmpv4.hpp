// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <system/net/ipv4/ipv4.hpp>
#include <stdint.h>

using namespace kernel::drivers::net;

namespace kernel::system::net::icmpv4 {

// TODO: Add more types
enum icmpv4Type
{
    TYPE_ECHO_REPLY = 0,
    TYPE_DEST_UNRCH = 3,
    TYPE_ECHO_REQST = 8,
};

struct [[gnu::packed]] icmpv4Hdr
{
    uint8_t type;
    uint8_t code;
    uint16_t csum;
    uint8_t data[];
};

struct [[gnu::packed]] icmpv4Echo
{
    uint16_t id;
    uint16_t seq;
    uint8_t data[];
};

struct [[gnu::packed]] icmpv4DstUnreach
{
    uint8_t unused;
    uint8_t len;
    uint16_t var;
    uint8_t data[];
};

extern bool debug;

void receive(nicmgr::NIC *nic, icmpv4Hdr *packet, ipv4::ipv4Hdr *iphdr);
}