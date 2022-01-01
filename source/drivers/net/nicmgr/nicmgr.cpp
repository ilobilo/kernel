// Copyright (C) 2021  ilobilo

#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/nicmgr/nicmgr.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::nicmgr {

bool initialised = false;
vector<NetCard*> nics;

void addCard(NetCard *card, type_t type)
{
    log("Registering network card #%zu", nics.size());
    nics.push_back(card);
    nics.front()->type = type;
    nics.front()->uniqueid = rand() % (RAND_MAX + 1 - 10000) + 10000;

}

void addRTL8139()
{
    if (!rtl8139::initialised) return;
    for (size_t i = 0; i < rtl8139::devices.size(); i++)
    {
        addCard(rtl8139::devices[i], RTL8139);
    }
}

void addE1000()
{
    if (!e1000::initialised) return;
    for (size_t i = 0; i < e1000::devices.size(); i++)
    {
        addCard(e1000::devices[i], E1000);
    }
}

void init()
{
    log("Initialising Network card manager");

    if (initialised)
    {
        warn("Network card manager has already been initialised!\n");
        return;
    }

    addRTL8139();
    addE1000();

    serial::newline();
    initialised = true;
}
}