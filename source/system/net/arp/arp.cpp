// Copyright (C) 2021-2022  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/shared_ptr.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::arp {

vector<tableEntry*> table;
bool debug = NET_DEBUG;

tableEntry *table_add(macaddr mac, ipv4addr ip)
{
    tableEntry *entry = new tableEntry;
    entry->mac = mac;
    entry->ip = ip;
    table.push_back(entry);
    return entry;
}

tableEntry *table_search(ipv4addr ip)
{
    for (size_t i = 0; i < table.size(); i++)
    {
        if (table[i]->ip == ip) return table[i];
    }
    return nullptr;
}

tableEntry *table_update(macaddr mac, ipv4addr ip)
{
    tableEntry *oldentry = table_search(ip);
    if (oldentry == nullptr) return table_add(mac, ip);
    oldentry->mac = mac;
    return oldentry;
}

void send(nicmgr::NIC *nic, macaddr dmac, ipv4addr dip)
{
    std::shared_ptr<arpHdr> packet(new arpHdr);

    packet->hwtype = HWTYPE_ETHERNET;
    packet->protype = ethernet::TYPE_IPv4;

    packet->hwsize = 6;
    packet->prosize = 4;

    packet->opcode = ARP_REQUEST;

    packet->smac = nic->MAC;
    packet->sip = nic->IPv4;

    packet->dmac = dmac;
    packet->dip = dip;

    ethernet::send(nic, dmac, reinterpret_cast<uint8_t*>(packet.get()), sizeof(arpHdr), ethernet::TYPE_ARP);

    if (debug) log("ARP: Packet sent!");
}

void reply(nicmgr::NIC *nic, arpHdr *packet)
{
    packet->dmac = packet->smac;
    packet->dip = packet->sip;

    packet->smac = nic->MAC;
    packet->sip = nic->IPv4;

    packet->opcode = ARP_REPLY;

    ethernet::send(nic, packet->dmac, reinterpret_cast<uint8_t*>(packet), sizeof(arpHdr), ethernet::TYPE_ARP);
}

void receive(nicmgr::NIC *nic, arpHdr *packet)
{
    if (bigendian<uint16_t>(packet->hwtype) != HWTYPE_ETHERNET)
    {
        if (debug) error("ARP: Unsupported hardware type!");
        return;
    }
    if (bigendian<uint16_t>(packet->protype) != ethernet::TYPE_IPv4)
    {
        if (debug) error("ARP: Unsupported protocol type!");
        return;
    }

    tableEntry *entry = table_search(packet->sip);
    if (entry == nullptr) entry = table_add(packet->smac, packet->sip);
    else entry->mac = packet->smac;

    switch (bigendian<uint16_t>(packet->opcode))
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