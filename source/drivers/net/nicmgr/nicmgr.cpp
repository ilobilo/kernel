// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/rtl8169/rtl8169.hpp>
#include <drivers/net/nicmgr/nicmgr.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::nicmgr {

bool initialised = false;
vector<NIC*> nics;

static uint8_t next = 50;
void addCard(NIC *card, type_t type)
{
    log("Registering NIC #%zu", nics.size());
    nics.push_back(card);
    nics.front()->type = type;
    nics.front()->uniqueid = rand() % (RAND_MAX + 1 - 10000) + 10000;

    // Temporarily hardcode IPv4 addresses
    // NIC 0 IPv4 = 192.168.0.50
    // NIC 0 IPv4 = 192.168.0.51
    // NIC 0 IPv4 = 192.168.0.52
    // ...

    card->IPv4 = { 192, 168, 0, next++ };
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

void addRTL8169()
{
    if (!rtl8169::initialised) return;
    for (size_t i = 0; i < rtl8169::devices.size(); i++)
    {
        addCard(rtl8169::devices[i], RTL8169);
        sprintf(nics.front()->name, "RTL8169 #%zu", i);
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
    addRTL8169();
    addE1000();

    serial::newline();
    initialised = true;
}
}