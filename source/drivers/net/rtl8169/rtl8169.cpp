// Copyright (C) 2021  ilobilo

#include <drivers/net/stack/ethernet/ethernet.hpp>
#include <drivers/net/rtl8169/rtl8169.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::net::stack;
using namespace kernel::system::cpu;

namespace kernel::drivers::net::rtl8169 {

bool initialised = false;
vector<RTL8169*> devices;

static void RTL8169_Handler(registers_t *regs)
{
    for (size_t i = 0; i < devices.size(); i++)
    {
        RTL8169 *device = devices[i];
        uint16_t status = device->status();
        if (status & (1 << 15)) error("RTL8169: Card #%zu: Error!", i);
        if (status & (1 << 14)) if (device->debug) warn("RTL8169: Card #%zu: Time out!", i);
        if (status & (1 << 8)) if (device->debug) warn("RTL8169: Card #%zu: Software interrupts!", i);
        if (status & (1 << 7)) if (device->debug) error("RTL8169: Card #%zu: TX buffer overflow!", i);
        if (status & (1 << 6)) if (device->debug) error("RTL8169: Card #%zu: RX FIFO overflow!", i);
        if (status & (1 << 5)) if (device->debug) log("RTL8169: Card #%zu: Link status change!", i);
        if (status & (1 << 4)) if (device->debug) error("RTL8169: Card #%zu: RX buffer overflow!", i);
        if (status & (1 << 3)) if (device->debug) error("RTL8169: Card #%zu: Error while sending packet!", i);
        if (status & (1 << 2)) if (device->debug) log("RTL8169: Card #%zu: Packet sent!", i);
        if (status & (1 << 1)) if (device->debug) error("RTL8169: Card #%zu: Error while receiving packet!", i);
        if (status & (1 << 0))
        {
            if (device->debug) log("RTL8169: Card #%zu: Packet received!", i);
            device->receive();
        }
        device->irq_reset(status);
    }
}

uint16_t RTL8169::status()
{
    return inw(this->IOBase + 0x3E);
}

void RTL8169::irq_reset(uint16_t status)
{
    outw(this->IOBase + 0x3E, status);
}

void RTL8169::read_mac()
{
    uint32_t mac1 = inl(this->IOBase + 0x00);
    uint16_t mac2 = inw(this->IOBase + 0x04);

    this->MAC[0] = mac1;
    this->MAC[1] = mac1 >> 8;
    this->MAC[2] = mac1 >> 16;
    this->MAC[3] = mac1 >> 24;
    this->MAC[4] = mac2;
    this->MAC[5] = mac2 >> 8;

    if (this->debug) log("MAC Address: %X:%X:%X:%X:%X:%X", this->MAC[0], this->MAC[1], this->MAC[2], this->MAC[3], this->MAC[4], this->MAC[5]);
}

void RTL8169::send(void *data, uint64_t length)
{
    acquire_lock(this->lock);
    void *tdata = malloc(length);
    memcpy(tdata, data, length);
    this->txdescs[this->txcurr]->buffer = reinterpret_cast<uint64_t>(tdata);
    this->txdescs[this->txcurr]->command = RTL8169_OWN | RTL8169_FFR | RTL8169_LFR | length;
    uint8_t old_cur = this->txcurr;
    if (++this->txcurr >= RTL8169_NUM_TX_DESC)
    {
        this->txcurr = 0;
        this->txdescs[old_cur]->command |= RTL8169_EOR;
    }
    outb(this->IOBase + 0x38, 0x40);
    free(tdata);
    release_lock(this->lock);
}

void RTL8169::receive()
{
    for (size_t i = 0; (inb(this->IOBase + 0x37) & 0x01) == 0; i++)
    {
        if (this->debug) log("RTL8169: Handling packet #%zu!", i);

        // size_t length = this->rxdescs[this->rxcurr]->command & 0xFFFF;
        size_t length = (this->rxdescs[this->rxcurr]->command & 0x1FFF) - 4;
        uint64_t t = this->rxdescs[this->rxcurr]->buffer;
        void *packet = malloc(length);
        memcpy(packet, reinterpret_cast<void*>(t), length);

        this->rxdescs[this->rxcurr]->command |= RTL8169_OWN;
        if (++this->rxcurr >= RTL8169_NUM_RX_DESC) this->rxcurr = 0;

        ethernet::receive(this, reinterpret_cast<ethernet::ethHdr*>(packet), length);
        free(packet);
    }
}

void RTL8169::rxinit()
{
    Desc *descs = static_cast<Desc*>(malloc(sizeof(Desc) * RTL8169_NUM_RX_DESC));
    for (size_t i = 0; i < RTL8169_NUM_RX_DESC; i++)
    {
        this->rxdescs[i] = reinterpret_cast<Desc*>(reinterpret_cast<uint8_t*>(descs) + i * 16);
        if (i == RTL8169_NUM_RX_DESC - 1) this->rxdescs[i]->command = RTL8169_OWN | RTL8169_EOR | (RTL8169_RX_BUFF_SIZE & 0x3FFF);
        else this->rxdescs[i]->command = RTL8169_OWN | (RTL8169_RX_BUFF_SIZE & 0x3FFF);
        this->rxdescs[i]->buffer = reinterpret_cast<uint64_t>(static_cast<uint8_t*>(malloc(RTL8169_RX_BUFF_SIZE + 16)));;
    }
}

void RTL8169::txinit()
{
    Desc *descs = static_cast<Desc*>(malloc(sizeof(Desc) * RTL8169_NUM_TX_DESC));
    for (size_t i = 0; i < RTL8169_NUM_TX_DESC; i++)
    {
        this->txdescs[i] = reinterpret_cast<Desc*>(reinterpret_cast<uint8_t*>(descs) + i * 16);
        if (i == RTL8169_NUM_TX_DESC - 1) this->rxdescs[i]->command = RTL8169_EOR;
    }
}

void RTL8169::reset()
{
    outb(IOBase + 0x37, 0x10);
    while ((inb(IOBase + 0x37) & 0x10));
}

void RTL8169::start()
{
    outb(this->IOBase + 0x52, 0x01);
    this->reset();

    this->rxinit();
    this->txinit();

    outb(this->IOBase + 0x50, 0xC0);
    outl(this->IOBase + 0x44, 0x0000E70F);
    outb(this->IOBase + 0x37, 0x04);
    outl(this->IOBase + 0x40, 0x03000700);
    outw(this->IOBase + 0xDA, 0x1FFF);
    outb(this->IOBase + 0xEC, 0x3B);
    outl(this->IOBase + 0x20, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->txdescs[0])));
    outl(this->IOBase + 0x24, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->txdescs[0]) >> 32));
    outl(this->IOBase + 0xE4, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->rxdescs[0])));
    outl(this->IOBase + 0xE8, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->rxdescs[0]) >> 32));
    outw(this->IOBase + 0x3C, 0xC1FF);
    outb(this->IOBase + 0x37, 0x0C);
    outb(this->IOBase + 0x50, 0x00);

    this->read_mac();
}

RTL8169::RTL8169(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering card #%zu", devices.size());

    pci::pcibar bar0 = pcidevice->get_bar(0);
    pci::pcibar bar1 = pcidevice->get_bar(1);
    this->BARType = bar1.mmio ? 0x00 : 0x01;
    this->MEMBase = bar1.address;
    this->IOBase = bar0.address;

    pcidevice->command(pci::CMD_BUS_MAST, true);

    this->start();

    pcidevice->irq_set(RTL8169_Handler);
}

uint16_t ids[6][2] = {
    { 0x10EC, 0x8161 },
    { 0x10EC, 0x8168 },
    { 0x10EC, 0x8169 },
    { 0x1259, 0xC107 },
    { 0x1737, 0x1032 },
    { 0x16EC, 0x0116 }
};

bool search(uint16_t vendorid, uint16_t deviceid)
{
    size_t count = pci::count(vendorid, deviceid);
    if (count == 0) return false;

    for (size_t i = 0; i < count; i++)
    {
        devices.push_back(new RTL8169(pci::search(vendorid, deviceid, i)));
        if (devices.front()->initialised == false)
        {
            free(devices.front());
            devices.pop_back();
        }
    }
    return true;
}

void init()
{
    log("Initialising RTL8169 driver");

    if (initialised)
    {
        warn("RTL8169 driver has already been initialised!\n");
        return;
    }

    bool found[6] = { [0 ... 5] = false };
    for (size_t i = 0; i < 6; i++)
    {
        found[i] = search(ids[i][0], ids[i][1]);
    }
    for (size_t i = 0; i < 6; i++)
    {
        if (found[i] == true)
        {
            serial::newline();
            if (devices.size() != 0) initialised = true;
            return;
        }
    }
    error("No RTL8169 cards found!\n");
}
}