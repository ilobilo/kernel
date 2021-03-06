// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <lib/net.hpp>
#include <cstdint>

using namespace kernel::drivers::net;

namespace kernel::system::net::ipv4 {

static constexpr uint8_t IP_TTL = 64;
static constexpr uint64_t IP_TRIES = 3;

enum version
{
    VER_IPv4 = 4,
    VER_IPv6 = 6,
};

enum ipv4Prot
{
    IPv4_PROT_ICMPv4 = 1,
    IPv4_PROT_TCP = 6,
    IPv4_PROT_UDP = 17,
};

struct [[gnu::packed]] ipv4Hdr
{
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t ecn : 2;
    uint8_t dscp : 6;
    bigendian<uint16_t> len;
    bigendian<uint16_t> id;
    uint16_t frag_offset : 13;
    uint8_t flags : 3;
    uint8_t ttl;
    uint8_t proto;
    bigendian<uint16_t> csum;
    ipv4addr sip;
    ipv4addr dip;
    uint8_t data[];
};

extern bool debug;

void send(nicmgr::NIC *nic, ipv4addr dip, void *data, size_t length, ipv4Prot protocol);
void receive(nicmgr::NIC *nic, ipv4Hdr *packet, macaddr smac);
}