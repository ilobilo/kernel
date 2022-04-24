// Copyright (C) 2021-2022  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <lib/shared_ptr.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::system::net;

namespace kernel::drivers::net::rtl8139 {

bool initialised = false;
vector<RTL8139*> devices;

static void RTL8139_Handler(registers_t *regs, uint64_t addr)
{
    auto device = reinterpret_cast<RTL8139*>(addr);

    uint16_t status = device->status();
    if (status & IST_SYSTEM_ERROR) error("RTL8139: Card #%zu: Error!", device->id);
    if (status & IST_TIME_OUT) if (device->debug) warn("RTL8139: Card #%zu: Time out!", device->id);
    if (status & IST_CABLE_LENGTH_CHANGE) if (device->debug) warn("RTL8139: Card #%zu: Cable length change!", device->id);
    if (status & IST_RX_BUFF_OVER) if (device->debug) error("RTL8139: Card #%zu: RX buffer overflow!", device->id);
    if (status & IST_TRANSMIT_ERR) if (device->debug) error("RTL8139: Card #%zu: Error while sending packet!", device->id);
    if (status & IST_TRANSMIT_OK) if (device->debug) log("RTL8139: Card #%zu: Packet sent!", device->id);
    if (status & IST_RECEIVE_ERR) if (device->debug) error("RTL8139: Card #%zu: Error while receiving packet!", device->id);
    if (status & IST_RECEIVE_OK)
    {
        if (device->debug) log("RTL8139: Card #%zu: Packet received!", device->id);
        device->receive();
    }
    device->irq_reset();
}

void RTL8139::outb(uint16_t addr, uint8_t val)
{
    if (this->BARType == 0) mmoutb(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else outb(this->IOBase + addr, val);
}
void RTL8139::outw(uint16_t addr, uint16_t val)
{
    if (this->BARType == 0) mmoutw(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else outw(this->IOBase + addr, val);
}
void RTL8139::outl(uint16_t addr, uint32_t val)
{
    if (this->BARType == 0) mmoutl(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else outl(this->IOBase + addr, val);
}
uint8_t RTL8139::inb(uint16_t addr)
{
    if (this->BARType == 0) return mminb(reinterpret_cast<void*>(this->MEMBase + addr));
    else return inb(this->IOBase + addr);
}
uint16_t RTL8139::inw(uint16_t addr)
{
    if (this->BARType == 0) return mminw(reinterpret_cast<void*>(this->MEMBase + addr));
    else return inw(this->IOBase + addr);
}
uint32_t RTL8139::inl(uint16_t addr)
{
    if (this->BARType == 0) return mminl(reinterpret_cast<void*>(this->MEMBase + addr));
    else return inl(this->IOBase + addr);
}

uint16_t RTL8139::status()
{
    return this->inw(REG_ISR);
}

void RTL8139::irq_reset()
{
    this->outw(REG_ISR, IST_RECEIVE_OK | IST_TRANSMIT_OK);
}

void RTL8139::read_mac()
{
    uint32_t mac1 = this->inl(REG_ID_0);
    uint16_t mac2 = this->inw(REG_ID_4);

    this->MAC[0] = mac1;
    this->MAC[1] = mac1 >> 8;
    this->MAC[2] = mac1 >> 16;
    this->MAC[3] = mac1 >> 24;
    this->MAC[4] = mac2;
    this->MAC[5] = mac2 >> 8;

    if (this->debug) log("MAC Address: %X:%X:%X:%X:%X:%X", this->MAC[0], this->MAC[1], this->MAC[2], this->MAC[3], this->MAC[4], this->MAC[5]);
}

void RTL8139::send(void *data, uint64_t length)
{
    lockit(this->lock);

    std::shared_ptr<uint8_t> tdata(new uint8_t[length]);
    memcpy(tdata.get(), data, length);
    this->outl(this->TSAD[this->txcurr], static_cast<uint32_t>(reinterpret_cast<uint64_t>(tdata.get())));
    this->outl(this->TSD[this->txcurr++], length);
    if (this->txcurr > 3) this->txcurr = 0;
}

void RTL8139::receive()
{
    uint16_t *t = reinterpret_cast<uint16_t*>(this->RXBuffer + this->current_packet);
    uint16_t length = *(t + 1);
    t += 2;
    std::shared_ptr<uint8_t> packet(new uint8_t[length]);
    memcpy(packet.get(), t, length);

    this->current_packet = (this->current_packet + length + 7) & ~3;
    if (this->current_packet > 8192) this->current_packet -= 8192;
    this->outw(REG_CAPR, this->current_packet - 0x10);

    ethernet::receive(this, reinterpret_cast<ethernet::ethHdr*>(packet.get()));
}

void RTL8139::reset()
{
    this->outb(REG_CR, CMD_RESET);
    while ((this->inb(REG_CR) & CMD_RESET));
}

void RTL8139::start()
{
    this->outb(REG_CONFIG1, 0x00);

    reset();

    RXBuffer = malloc<uint8_t*>(8192 + 16 + 1500);
    this->outl(REG_RBSTART, static_cast<uint32_t>(reinterpret_cast<uint64_t>(RXBuffer)));

    this->outw(REG_IMR, IMR_RECEIVE_OK | IMR_RECEIVE_ERROR | IMR_TRANSMIT_OK | IMR_TRANSMIT_ERROR | IMR_RX_OVERFLOW | IMR_LINK_CHANGE | IMR_RX_FIFO_OVERFLOW | IMR_CABLE_LENGTH_CHANGE | IMR_TIME_OUT | IMR_SYSTEM_ERROR);
    this->outl(REG_RCR, RCR_PHYS_ADDR_PACKETS | RCR_PHYS_MATCH_PACKETS | RCR_MULTICAST_PACKETS | RCR_BROADCAST_PACKETS | RCR_WRAP);
    this->outb(REG_CR, 0x0C);

    this->read_mac();
}

RTL8139::RTL8139(pci::pcidevice_t *pcidevice, uint8_t id)
{
    this->pcidevice = pcidevice;
    this->id = id;
    log("Registering card #%zu", id);

    pci::pcibar bar0 = pcidevice->get_bar(0);
    pci::pcibar bar1 = pcidevice->get_bar(1);
    this->BARType = bar1.mmio ? 0x00 : 0x01;
    this->MEMBase = bar1.address;
    this->IOBase = bar0.address;

    pcidevice->command(pci::CMD_BUS_MAST | pci::CMD_IO_SPACE | pci::CMD_MEM_SPACE, true);

    this->start();

    pcidevice->irq_set(RTL8139_Handler, reinterpret_cast<uint64_t>(this));

    this->initialised = true;
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
        devices.push_back(new RTL8139(pci::search(0x10EC, 0x8139, i), devices.size()));
        if (devices.back()->initialised == false)
        {
            free(devices.back());
            devices.pop_back();
            error("Could not initialise RTL8139 driver #%zu", devices.size());
        }
    }

    serial::newline();
    if (devices.size() != 0) initialised = true;
}
}