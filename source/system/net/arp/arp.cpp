// Copyright (C) 2021-2022  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::arp {

vector<tableEntry*> table;
bool debug = NET_DEBUG;

tableEntry *table_add(uint8_t *mac, uint8_t *ip)
{
    tableEntry *entry = new tableEntry;
    memcpy(entry->mac, mac, 6);
    memcpy(entry->ip, ip, 4);
    table.push_back(entry);
    return entry;
}

tableEntry *table_search(uint8_t *ip)
{
    for (size_t i = 0; i < table.size(); i++)
    {
        if (arraycmp(table[i]->ip, ip, 4)) return table[i];
    }
    return nullptr;
}

tableEntry *table_update(uint8_t *mac, uint8_t *ip)
{
    tableEntry *oldentry = table_search(ip);
    if (oldentry == nullptr) return table_add(mac, ip);
    memcpy(oldentry->mac, mac, 6);
    return oldentry;
}

void send(nicmgr::NIC *nic, uint8_t *dmac, uint8_t *dip)
{
    arpHdr *packet = new arpHdr;

    packet->hwtype = htons(HWTYPE_ETHERNET);
    packet->protype = htons(ethernet::TYPE_IPv4);

    packet->hwsize = 6;
    packet->prosize = 4;

    packet->opcode = htons(ARP_REQUEST);

    memcpy(packet->ip.smac, nic->MAC, 6);
    memcpy(packet->ip.sip, nic->IPv4, 4);

    memcpy(packet->ip.dmac, dmac, 6);
    memcpy(packet->ip.dip, dip, 4);

    ethernet::send(nic, dmac, reinterpret_cast<uint8_t*>(packet), sizeof(arpHdr), ethernet::TYPE_ARP);
    delete packet;

    if (debug) log("ARP: Packet sent!");
}

void reply(nicmgr::NIC *nic, arpHdr *packet)
{
    memcpy(packet->ip.dmac, packet->ip.smac, 6);
    memcpy(packet->ip.dip, packet->ip.sip, 4);

    memcpy(packet->ip.smac, nic->MAC, 6);
    memcpy(packet->ip.sip, nic->IPv4, 4);

    packet->opcode = htons(ARP_REPLY);

    ethernet::send(nic, packet->ip.dmac, reinterpret_cast<uint8_t*>(packet), sizeof(arpHdr), ethernet::TYPE_ARP);
}

void receive(nicmgr::NIC *nic, arpHdr *packet)
{
    if (ntohs(packet->hwtype) != HWTYPE_ETHERNET)
    {
        if (debug) error("ARP: Unsupported hardware type!");
        return;
    }
    if (ntohs(packet->protype) != ethernet::TYPE_IPv4)
    {
        if (debug) error("ARP: Unsupported protocol type!");
        return;
    }

    tableEntry *entry = table_search(packet->ip.sip);
    if (entry == nullptr) entry = table_add(packet->ip.smac, packet->ip.sip);
    else memcpy(entry->mac, packet->ip.smac, 6);

    if (!arraycmp(nic->IPv4, packet->ip.dip, 4))
    {
        if (debug) error("ARP: Packet is not for us!");
        return;
    }

    switch (ntohs(packet->opcode))
    {
        case ARP_REQUEST:
            if (debug) log("ARP: Request!");
            reply(nic, packet);
            break;
        case ARP_REPLY:
            if (debug)
            {
                log("ARP: Reply!");
                error("Not supported!");
            }
            break;
        default:
            if (debug) error("ARP: Opcode not supported!");
            break;
    }
}
}