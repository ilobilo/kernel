// Copyright (C) 2021  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/ipv4/ipv4.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::ipv4 {

bool debug = NET_DEBUG;

static uint16_t checksum(void *addr, size_t size)
{
    uint32_t sum = 0;
    uint16_t *ptr = static_cast<uint16_t*>(addr);

    while (size > 1)
    {
        sum += *ptr++;
        size -= 2;
    }
    if (size > 0) sum += *reinterpret_cast<uint8_t*>(ptr);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

void send(nicmgr::NIC *nic, uint8_t *dip, void *data, size_t length, ipv4Prot protocol)
{
    ipv4Hdr *packet = static_cast<ipv4Hdr*>(malloc(length + sizeof(ipv4Hdr)));

    packet->version = IPv4_VER;
    packet->ihl = sizeof(ipv4Hdr) / 4;
    packet->tos = 0;
    packet->len = htons(sizeof(ipv4Hdr) + length);
    packet->id = 0;
    packet->flags = 0;
    packet->frag_offset = 0;
    packet->ttl = IPv4_TTL;
    packet->proto = protocol;
    packet->csum = 0;
    memcpy(packet->sip, nic->IPv4, 4);
    memcpy(packet->sip, dip, 4);

    memcpy(packet->data, data, length);

    packet->csum = htons(checksum(packet, sizeof(ipv4Hdr)));

    uint8_t dmac[6];
    arp::tableEntry *entry = nullptr;

    for (size_t i = IPv4_TRIES; i > 0 && entry == nullptr; i--)
    {
        entry = arp::table_search(dip);
    }
    if (entry != nullptr) memcpy(dmac, entry->mac, 6);

    ethernet::send(nic, dmac, reinterpret_cast<uint8_t*>(packet), sizeof(ipv4Hdr) + length, ethernet::TYPE_IPv4);
    free(packet);

    if (debug) log("IPv4: Packet sent!");
}

void receive(nicmgr::NIC *nic, ipv4Hdr *packet)
{
    switch (packet->version)
    {
        case IPv4_VER:
            if (debug) log("IPv4: Version 4");
            switch (packet->proto)
            {
                case IPv4_PROT_ICMP:
                    if (debug) log("IPv4: Detected ICMP packet!");
                    break;
                case IPv4_PROT_TCP:
                    if (debug) log("IPv4: Detected TCP packet!");
                    break;
                case IPv4_PROT_UDP:
                    if (debug) log("IPv4: Detected UDP packet!");
                    break;
            }
            break;
        default:
            if (debug) error("IPv4: Unsupported version!");
            break;
    }
}
}