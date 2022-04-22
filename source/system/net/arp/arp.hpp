// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <lib/net.hpp>
#include <cstdint>

using namespace kernel::drivers::net;

namespace kernel::system::net::arp {

enum opcodes
{
    ARP_REQUEST = 0x01,
    ARP_REPLY = 0x02,
};

enum hwtypes
{
    HWTYPE_ETHERNET = 0x01
};

struct [[gnu::packed]] arpHdr
{
    bigendian<uint16_t> hwtype;
    bigendian<uint16_t> protype;
    uint8_t hwsize;
    uint8_t prosize;
    bigendian<uint16_t> opcode;
    macaddr smac;
    ipv4addr sip;
    macaddr dmac;
    ipv4addr dip;
};

struct tableEntry
{
    macaddr mac;
    ipv4addr ip;
};

extern vector<tableEntry*> table;
extern bool debug;

tableEntry *table_add(macaddr mac, ipv4addr ip);
tableEntry *table_search(ipv4addr ip);
tableEntry *table_update(macaddr mac, ipv4addr ip);

void send(nicmgr::NIC *nic, macaddr dmac, ipv4addr dip);
void receive(nicmgr::NIC *nic, arpHdr *packet);
}