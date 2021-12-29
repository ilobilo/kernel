// Copyright (C) 2021  ilobilo

#include <drivers/net/am79c970a/am79c970a.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/cardmgr/cardmgr.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::net::cardmgr {

bool initialised = false;
vector<NetCard*> cards;

void addCard(NetCard *card, type_t type)
{
    log("Registering network card #%zu", cards.size());
    cards.push_back(card);
    cards.front()->type = type;
}

void addRTL8139()
{
    if (!rtl8139::initialised) return;
    for (size_t i = 0; i < rtl8139::devices.size(); i++)
    {
        addCard(rtl8139::devices[i], RTL8139);
    }
}

void addAM79C970A()
{
    if (!am79c970a::initialised) return;
    for (size_t i = 0; i < am79c970a::devices.size(); i++)
    {
        addCard(am79c970a::devices[i], AM79C970A);
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
    addAM79C970A();
    addE1000();

    cards[0]->send(new char[10], 10);

    serial::newline();
    initialised = true;
}
}