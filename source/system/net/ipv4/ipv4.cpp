// Copyright (C) 2021-2022  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/icmpv4/icmpv4.hpp>
#include <system/net/ipv4/ipv4.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::ipv4 {

bool debug = NET_DEBUG;

void send(nicmgr::NIC *nic, ipv4addr dip, void *data, size_t length, ipv4Prot protocol)
{
    ipv4Hdr *packet = malloc<ipv4Hdr*>(length + sizeof(ipv4Hdr));

    packet->version = VER_IPv4;
    packet->ihl = 5;
    packet->len = sizeof(ipv4Hdr) + length;
    packet->flags = 0;
    packet->ttl = IP_TTL;
    packet->proto = protocol;
    packet->csum = 0;
    packet->sip = nic->IPv4;
    packet->dip = dip;

    packet->csum = checksum(packet, packet->ihl * 4);

    memcpy(packet->data, data, length);

    macaddr dmac;
    arp::tableEntry *entry = arp::table_search(dip);

    for (size_t i = IP_TRIES; i > 0 && entry == nullptr; i--)
    {
        arp::send(nic, macaddr(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF), dip);
        entry = arp::table_search(dip);
    }
    if (entry != nullptr) dmac = entry->mac;
    else
    {
        error("IPv4: Could not find destination MAC address!");
        return;
    }

    ethernet::send(nic, dmac, reinterpret_cast<uint8_t*>(packet), length + sizeof(ipv4Hdr), ethernet::TYPE_IPv4);
    free(packet);

    if (debug) log("IPv4: Packet sent!");
}

void receive(nicmgr::NIC *nic, ipv4Hdr *packet, macaddr smac)
{
    arp::table_update(smac, packet->sip);
    // if (!arraycmp(nic->IPv4, packet->dip, 4))
    // {
    //     if (debug) error("IPv4: Packet is not for us!");
    //     return;
    // }

    if (packet->ihl < 5)
    {
        if (debug) error("IPv4: Packet IHL must be at least 5!");
        return;
    }
    if (packet->ttl == 0)
    {
        if (debug) error("IPv4: TTL has reached 0!");
        return;
    }

    bigendian<uint16_t> csum = packet->csum;
    packet->csum = 0;
    if (csum.value != checksum(packet, sizeof(ipv4Hdr)).value)
    {
        if (debug) error("IPv4: Invalid checksum!");
        return;
    }

    switch (packet->version)
    {
        case VER_IPv4:
            if (debug) log("IPv4: Version 4");
            switch (packet->proto)
            {
                case IPv4_PROT_ICMPv4:
                    if (debug) log("IPv4: Detected ICMP packet!");
                    icmpv4::receive(nic, reinterpret_cast<icmpv4::icmpv4Hdr*>(packet->data), packet);
                    break;
                case IPv4_PROT_TCP:
                    if (debug) log("IPv4: Detected TCP packet!");
                    break;
                case IPv4_PROT_UDP:
                    if (debug) log("IPv4: Detected UDP packet!");
                    break;
                default:
                    if (debug) error("IPv4: Invalid protocol %d!", packet->proto);
                    break;
            }
            break;
        default:
            if (debug) error("IPv4: Unsupported version %d!", packet->version);
            break;
    }
}
}