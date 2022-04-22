// Copyright (C) 2021-2022  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/ipv4/ipv4.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::ethernet {

bool debug = NET_DEBUG;

void send(nicmgr::NIC *nic, macaddr dmac, void *data, size_t length, uint16_t protocol)
{
    ethHdr *frame = malloc<ethHdr*>(sizeof(ethHdr) + length);

    frame->smac = nic->MAC;
    frame->dmac = dmac;
    frame->type = protocol;

    memcpy(frame->data, data, length);

    nic->send(reinterpret_cast<uint8_t*>(frame), sizeof(ethHdr) + length);
    free(frame);
    if (debug) log("Ethernet: Packet sent!");
}

void receive(nicmgr::NIC *nic, ethHdr *packet)
{
    if (packet->dmac != nic->MAC && packet->dmac != macaddr(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF)) return;

    switch (packet->type)
    {
        case TYPE_ARP:
            if (debug) log("Ethernet: Received ARP packet!");
            arp::receive(nic, reinterpret_cast<arp::arpHdr*>(packet->data));
            break;
        case TYPE_IPv4:
            if (debug) log("Ethernet: Received IPv4 packet!");
            ipv4::receive(nic, reinterpret_cast<ipv4::ipv4Hdr*>(packet->data), packet->smac);
            break;
    }
}
}