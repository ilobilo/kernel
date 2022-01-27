// Copyright (C) 2021  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <system/net/icmpv4/icmpv4.hpp>
#include <system/net/arp/arp.hpp>
#include <lib/memory.hpp>
#include <lib/net.hpp>
#include <lib/log.hpp>

namespace kernel::system::net::icmpv4 {

bool debug = NET_DEBUG;

void reply(nicmgr::NIC *nic, uint8_t *dip, icmpv4Hdr *oldpacket, size_t length)
{
    icmpv4Hdr *newpacket = static_cast<icmpv4Hdr*>(malloc(length));

    newpacket->type = TYPE_ECHO_REPLY;
    newpacket->code = 0;
    newpacket->csum = 0;

    icmpv4Echo *oldecho = reinterpret_cast<icmpv4Echo*>(oldpacket->data);
    icmpv4Echo *newecho = reinterpret_cast<icmpv4Echo*>(newpacket->data);

    newecho->id = oldecho->id;
    newecho->seq = oldecho->seq;
    memcpy(newecho->data, oldecho->data, length - sizeof(icmpv4Hdr) - sizeof(icmpv4Echo));

    newpacket->csum = checksum(newpacket, length);

    ipv4::send(nic, dip, newpacket, length, ipv4::IPv4_PROT_ICMPv4);
    free(newpacket);
}

void receive(nicmgr::NIC *nic, icmpv4Hdr *packet, ipv4::ipv4Hdr *iphdr)
{
    switch (packet->type)
    {
        case TYPE_ECHO_REPLY:
            if (debug) log("ICMPv4: Echo reply!");
            break;
        case TYPE_ECHO_REQST:
            if (debug) log("ICMPv4: Echo request!");
            reply(nic, iphdr->sip, packet, iphdr->len - iphdr->ihl * 4);
            break;
        case TYPE_DEST_UNRCH:
            if (debug) error("ICMPv4: Destination unreachable!");
            break;
        default:
            if (debug) error("ICMPv4: Unsupported type %d!", packet->type);
            break;
    }
}
}