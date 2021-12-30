// Copyright (C) 2021  ilobilo

#include <drivers/net/stack/ethernet/ethernet.hpp>
#include <drivers/net/stack/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::stack::arp {

uint8_t broadcastMAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t broadcastIP[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
vector<tableEntry*> table;
static bool init = false;

void table_add(uint8_t *mac, uint8_t *ip)
{
    tableEntry *entry = new tableEntry;
    memcpy(entry->mac, mac, 6);
    memcpy(entry->ip, ip, 4);
    table.push_back(entry);
}

bool table_search(uint8_t *mac, uint8_t *ip)
{
    if (!init)
    {
        table_add(broadcastMAC, broadcastIP);
        init = true;
    }
    for (size_t i = 0; i < table.size(); i++)
    {
        if (!memcmp(table[i]->ip, ip, 4))
        {
            memcpy(mac, table[i]->mac, 6);
            return true;
        }
    }
    return false;
}

void send(nicmgr::NetCard *nic, uint8_t *dmac, uint8_t *dip)
{
    if (!init)
    {
        table_add(broadcastMAC, broadcastIP);
        init = true;
    }
    arpHdr *packet = new arpHdr;

    memcpy(packet->ip.smac, nic->MAC, 6);
    memcpy(packet->ip.sip, nic->IPv4, 4);
    memcpy(packet->ip.dmac, dmac, 6);
    memcpy(packet->ip.dip, dip, 4);

    packet->opcode = htons(ARP_REQUEST);

    packet->hwsize = 6;
    packet->prosize = 4;
    packet->hwtype = htons(HWTYPE_ETHERNET);
    packet->protype = htons(ethernet::TYPE_IP);

    ethernet::send(nic, broadcastMAC, reinterpret_cast<uint8_t*>(packet), sizeof(arpHdr), ethernet::TYPE_ARP);
}

void recive(nicmgr::NetCard *nic, arpHdr *packet, size_t length)
{
    if (!init)
    {
        table_add(broadcastMAC, broadcastIP);
        init = true;
    }
    if (memcmp(packet->ip.dip, nic->IPv4, 4)) return;

    uint8_t dmac[6];
    uint8_t dip[4];

    memcpy(dmac, packet->ip.smac, 6);
    memcpy(dip, packet->ip.sip, 4);

    switch (ntohs(packet->opcode))
    {
        case ARP_REQUEST:
            log("ARP: Request!");

            memcpy(packet->ip.dmac, dmac, 6);
            memcpy(packet->ip.dip, dip, 4);

            memcpy(packet->ip.smac, nic->MAC, 6);
            memcpy(packet->ip.sip, nic->IPv4, 4);

            packet->opcode = htons(ARP_REPLY);

            packet->hwsize = 6;
            packet->prosize = 4;
            packet->hwtype = htons(HWTYPE_ETHERNET);
            packet->protype = htons(ethernet::TYPE_IP);

            ethernet::send(nic, dmac, reinterpret_cast<uint8_t*>(packet), sizeof(arpHdr), ethernet::TYPE_ARP);
            break;
        case ARP_REPLY:
            log("ARP: Reply!");
            break;
    }

    table_add(dmac, dip);
}
}