// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <stdint.h>

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

struct [[gnu::packed]] ipv4
{
    uint8_t smac[6];
    uint8_t sip[4];
    uint8_t dmac[6];
    uint8_t dip[4];
};

struct [[gnu::packed]] arpHdr
{
    uint16_t hwtype;
    uint16_t protype;
    uint8_t hwsize;
    uint8_t prosize;
    uint16_t opcode;
    ipv4 ip;
};

struct tableEntry
{
    uint8_t mac[6];
    uint8_t ip[4];
};

extern vector<tableEntry*> table;
extern bool debug;

tableEntry *table_add(uint8_t *mac, uint8_t *ip);
tableEntry *table_search(uint8_t *ip);
tableEntry *table_update(uint8_t *mac, uint8_t *ip);

void send(nicmgr::NIC *nic, uint8_t *dmac, uint8_t *dip);
void receive(nicmgr::NIC *nic, arpHdr *packet);
}