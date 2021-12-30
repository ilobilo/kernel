// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <stdint.h>

namespace kernel::drivers::net::stack::arp {

enum opcodes
{
    ARP_REQUEST = 1,
    ARP_REPLY = 2,
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

extern uint8_t broadcastMAC[];
extern vector<tableEntry*> table;

void table_add(uint8_t *mac, uint8_t *ip);
bool table_search(uint8_t *mac, uint8_t *ip);

void send(nicmgr::NetCard *nic, uint8_t *dmac, uint8_t *dip);
void receive(nicmgr::NetCard *nic, arpHdr *packet, size_t length);
}