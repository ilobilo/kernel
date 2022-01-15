// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::net::rtl8139 {

class RTL8139 : public nicmgr::NIC
{
    private:
    pci::pcidevice_t *pcidevice;
    volatile lock_t lock = false;

    uint8_t BARType = 0;
    uint16_t IOBase = 0;
    uint64_t MEMBase = 0;

    uint8_t *RXBuffer = nullptr;
    uint32_t current_packet = 0;

    uint8_t TSAD[4] = { 0x20, 0x24, 0x28, 0x2C };
    uint8_t TSD[4] = { 0x10, 0x14, 0x18, 0x1C };
    size_t txcurr = 0;

    public:
    bool initialised = false;

    void send(void *data, uint64_t length);
    void receive();

    void reset();
    void start();

    uint16_t status();
    void irq_reset();

    void read_mac();

    RTL8139(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<RTL8139*> devices;

void init();
}