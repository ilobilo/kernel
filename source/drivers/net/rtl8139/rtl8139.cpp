// Copyright (C) 2021  ilobilo

#include <drivers/net/stack/ethernet/ethernet.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::net::stack;
using namespace kernel::system::cpu;

namespace kernel::drivers::net::rtl8139 {

bool initialised = false;
static bool first = true;
vector<RTL8139*> devices;

static void RTL8139_Handler(registers_t *regs)
{
    for (size_t i = 0; i < devices.size(); i++)
    {
        RTL8139 *device = devices[i];
        uint16_t status = device->status();
        if (status & (1 << 2)) log("RTL8139: Card #%zu: Packet sent!", i);
        if (status & (1 << 0))
        {
            device->recive();
            log("RTL8139: Card #%zu: Packet recived!", i);
        }
        device->irq_reset();
    }
}

uint16_t RTL8139::status()
{
    return inw(this->IOBase + 0x3E);
}

void RTL8139::irq_reset()
{
    outw(this->IOBase + 0x3E, 0x05);
}

void RTL8139::read_mac()
{
    uint32_t mac1 = inl(IOBase + 0x00);
    uint16_t mac2 = inw(IOBase + 0x04);

    this->MAC[0] = mac1;
    this->MAC[1] = mac1 >> 8;
    this->MAC[2] = mac1 >> 16;
    this->MAC[3] = mac1 >> 24;
    this->MAC[4] = mac2;
    this->MAC[5] = mac2 >> 8;

    log("MAC Address: %X:%X:%X:%X:%X:%X", this->MAC[0], this->MAC[1], this->MAC[2], this->MAC[3], this->MAC[4], this->MAC[5]);
}

void RTL8139::send(uint8_t *data, uint64_t length)
{
    void *tdata = malloc(length);
    memcpy(tdata, data, length);
    outl(this->IOBase + this->TSAD[this->curr_tx], static_cast<uint32_t>(reinterpret_cast<uint64_t>(tdata)));
    outl(this->IOBase + this->TSD[this->curr_tx++], length);
    if (this->curr_tx > 3) this->curr_tx = 0;
    free(tdata);
}

void RTL8139::recive()
{
    uint16_t *t = reinterpret_cast<uint16_t*>(this->RXBuffer + this->current_packet);
    uint16_t length = *(t + 1);
    t += 2;
    void *packet = malloc(length);
    memcpy(packet, t, length);

    this->current_packet = (this->current_packet + length + 7) & ~3;
    if (this->current_packet > 8192) this->current_packet -= 8192;
    outw(this->IOBase + 0x38, this->current_packet - 0x10);

    ethernet::recive(this, reinterpret_cast<ethernet::ethHdr*>(packet), length);
    free(packet);
}

void RTL8139::reset()
{
    outb(IOBase + 0x37, 0x10);
    while ((inb(IOBase + 0x37) & 0x10));
}

void RTL8139::start()
{
    outb(IOBase + 0x52, 0x00);

    reset();

    RXBuffer = static_cast<uint8_t*>(malloc(8192 + 16 + 1500));
    outl(IOBase + 0x30, static_cast<uint32_t>(reinterpret_cast<uint64_t>(RXBuffer)));

    outw(IOBase + 0x3C, 0x05);
    outl(IOBase + 0x44, 0x0F | (1 << 7));
    outb(IOBase + 0x37, 0x0C);
}

RTL8139::RTL8139(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering RTL8139 driver #%zu", devices.size());

    uint32_t BAR0 = 0;
    if (pci::legacy) BAR0 = pcidevice->readl(pci::PCI_BAR0);
    else BAR0 = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->BAR0;

    this->BARType = BAR0 & 0x01;
    this->IOBase = pcidevice->get_bar(PCI_BAR_IO) & ~1;

    pcidevice->command(pci::CMD_BUS_MAST, true);

    this->start();

    uint8_t IRQ = 0;
    if (pci::legacy) IRQ = pcidevice->readl(pci::PCI_INTERRUPT_LINE);
    else IRQ = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->intLine;

    if (first)
    {
        first = false;
        idt::register_interrupt_handler(IRQ + 32, RTL8139_Handler);
    }

    this->read_mac();
}

void init()
{
    log("Initialising RTL8139 driver");

    if (initialised)
    {
        warn("RTL8139 driver has already been initialised!\n");
        return;
    }

    size_t count = pci::count(0x10EC, 0x8139);
    if (count == 0)
    {
        error("No RTL8139 cards found!\n");
        return;
    }

    devices.init(count);
    for (size_t i = 0; i < count; i++)
    {
        devices.push_back(new RTL8139(pci::search(0x10EC, 0x8139, i)));
    }

    serial::newline();
    initialised = true;
}
}