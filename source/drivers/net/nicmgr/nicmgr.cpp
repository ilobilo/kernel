// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/nicmgr/nicmgr.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::nicmgr {

bool initialised = false;
vector<NIC*> nics;


void addCard(NIC *card, type_t type)
{
    log("Registering NIC #%zu", nics.size());
    nics.push_back(card);
    nics.front()->type = type;
    nics.front()->uniqueid = rand() % (RAND_MAX + 1 - 10000) + 10000;

    // Temporarily hardcode IPv4 addresses
    // NIC 0 IPv4 = 10.2.2.2
    // NIC 0 IPv4 = 10.2.2.3
    // NIC 0 IPv4 = 10.2.2.4
    // ...

    static uint8_t i = 0x02;
    uint8_t ip[4] = { 0x10, 0x02, 0x02, i };
    memcpy(card->IPv4, ip, 4);
    i++;
}

void addRTL8139()
{
    if (!rtl8139::initialised) return;
    for (size_t i = 0; i < rtl8139::devices.size(); i++)
    {
        addCard(rtl8139::devices[i], RTL8139);
        sprintf(nics.front()->name, "RTL8139 #%zu", i);
    }
}

void addE1000()
{
    if (!e1000::initialised) return;
    for (size_t i = 0; i < e1000::devices.size(); i++)
    {
        addCard(e1000::devices[i], E1000);
        sprintf(nics.front()->name, "E1000 #%zu", i);
    }
}

void init()
{
    log("Initialising NIC manager");

    if (initialised)
    {
        warn("NIC manager has already been initialised!\n");
        return;
    }

    addRTL8139();
    addE1000();

    serial::newline();
    initialised = true;
}
}