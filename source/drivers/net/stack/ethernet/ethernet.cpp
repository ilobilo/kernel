// Copyright (C) 2021  ilobilo

#include <drivers/net/stack/ethernet/ethernet.hpp>
#include <drivers/net/stack/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::stack::ethernet {

bool debug = NET_DEBUG;

void send(nicmgr::NIC *nic, uint8_t *dmac, uint8_t *data, size_t length, uint16_t protocol)
{
    ethHdr *frame = static_cast<ethHdr*>(malloc(sizeof(ethHdr) + length));
    void *frmdata = reinterpret_cast<uint8_t*>(frame) + sizeof(ethHdr);

    memcpy(frame->smac, nic->MAC, 6);
    memcpy(frame->dmac, dmac, 6);
    memcpy(frmdata, data, length);
    frame->type = htons(protocol);

    nic->send(reinterpret_cast<uint8_t*>(frame), sizeof(ethHdr) + length);
    free(frame);
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
            if (debug) log("Ethernet: Received IP packet!");
            // ipv4::receive(nic, reinterpret_cast<ipv4::ipv4Hdr*>(data));
            break;
    }
}
}