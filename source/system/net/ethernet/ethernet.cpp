// Copyright (C) 2021  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/ipv4/ipv4.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::ethernet {

bool debug = NET_DEBUG;

void send(nicmgr::NIC *nic, uint8_t *dmac, void *data, size_t length, uint16_t protocol)
{
    ethHdr *frame = static_cast<ethHdr*>(malloc(sizeof(ethHdr) + length));

    memcpy(frame->smac, nic->MAC, 6);
    memcpy(frame->dmac, dmac, 6);
    memcpy(frame->data, data, length);
    frame->type = htons(protocol);

    nic->send(reinterpret_cast<uint8_t*>(frame), sizeof(ethHdr) + length);
    free(frame);
    if (debug) log("Ethernet: Packet sent!");
}

void receive(nicmgr::NIC *nic, ethHdr *packet, [[gnu::unused]] size_t length)
{
    void *data = reinterpret_cast<uint8_t*>(packet) + sizeof(ethHdr);
    length -= sizeof(ethHdr);
    uint16_t realtype = ntohs(packet->type);

    switch (realtype)
    {
        case TYPE_ARP:
            if (debug) log("Ethernet: Received ARP packet!");
            arp::receive(nic, reinterpret_cast<arp::arpHdr*>(data));
            break;
        case TYPE_IPv4:
            if (debug) log("Ethernet: Received IPv4 packet!");
            ipv4::receive(nic, reinterpret_cast<ipv4::ipv4Hdr*>(data), packet->smac);
            break;
    }
}
}