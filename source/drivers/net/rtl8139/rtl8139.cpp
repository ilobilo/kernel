// Copyright (C) 2021  ilobilo

#include <drivers/net/rtl8139/rtl8139.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/pci/pci.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::system::cpu;
using namespace kernel::system;

namespace kernel::drivers::net::rtl8139 {

bool initialised = false;
static pci::pcidevice_t *pcidevice = nullptr;
static uint16_t IOBase = 0;
static uint8_t BARType = 0;
static uint8_t *RXBuffer = nullptr;
static uint32_t current_packet = 0;
static uint8_t TSAD[4] = { 0x20, 0x24, 0x28, 0x2C };
static uint8_t TSD[4] = { 0x10, 0x14, 0x18, 0x1C };
size_t curr_tx = 0;
uint8_t MAC[6];

void send(void *data, uint64_t length)
{
    void *tdata = malloc(length);
    memcpy(tdata, data, length);
    outl(IOBase + TSAD[curr_tx], static_cast<uint32_t>(reinterpret_cast<uint64_t>(tdata)));
    outl(IOBase + TSD[curr_tx++], length);
    if (curr_tx > 3) curr_tx = 0;
    free(tdata);
}

void recive()
{
    uint16_t *t = reinterpret_cast<uint16_t*>(RXBuffer + current_packet);
    uint16_t length = *(t + 1);
    t += 2;
    void *packet = malloc(length);
    memcpy(packet, t, length);
    current_packet = (current_packet + length + 7) & ~3;
    if (current_packet > 8192) current_packet -= 8192;
    outw(IOBase + 0x38, current_packet - 0x10);
}

static void RTL8139_Handler(registers_t *regs)
{
    uint16_t status = inw(IOBase + 0x3E);
    if (status & (1 << 2)) log("RTL8139: Packet sent!");
    if (status & (1 << 0)) recive();
    outw(IOBase + 0x3E, 0x05);
}

void read_mac()
{
    uint32_t mac1 = inl(IOBase + 0x00);
    uint16_t mac2 = inw(IOBase + 0x04);

    MAC[0] = mac1;
    MAC[1] = mac1 >> 8;
    MAC[2] = mac1 >> 16;
    MAC[3] = mac1 >> 24;
    MAC[4] = mac2;
    MAC[5] = mac2 >> 8;

    log("RTL8139: MAC Address: %X:%X:%X:%X:%X:%X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
}

void reset()
{
    outb(IOBase + 0x37, 0x10);
    while ((inb(IOBase + 0x37) & 0x10));
}

void activate()
{
    outb(IOBase + 0x52, 0x00);

    reset();

    RXBuffer = static_cast<uint8_t*>(malloc(8192 + 16 + 1500));
    outl(IOBase + 0x30, static_cast<uint32_t>(reinterpret_cast<uint64_t>(RXBuffer)));

    outw(IOBase + 0x3C, 0x05);
    outl(IOBase + 0x44, 0x0F | (1 << 7));
    outb(IOBase + 0x37, 0x0C);
}

void init()
{
    log("Initialising RTL8139");

    if (initialised)
    {
        warn("RTL8139 has already been initialised!\n");
        return;
    }

    pcidevice = pci::search(0x10EC, 0x8139, 0);
    if (!pcidevice)
    {
        error("RTL8139 card not found!");
        return;
    }

    uint32_t BAR0 = 0;
    if (pci::legacy) BAR0 = pcidevice->readl(PCI_BAR0);
    else BAR0 = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->BAR0;

    BARType = BAR0 & 0x01;
    IOBase = BAR0 & (~0x03);

    pcidevice->bus_mastering(true);

    activate();

    uint8_t IRQ = 0;
    if (pci::legacy) IRQ = pcidevice->readl(PCI_INTERRUPT_LINE);
    else IRQ = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->intLine;

    idt::register_interrupt_handler(IRQ + 32, RTL8139_Handler);

    read_mac();

    serial::newline();
    initialised = true;
}
}