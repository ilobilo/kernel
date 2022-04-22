// Copyright (C) 2021-2022  ilobilo

#include <system/net/ethernet/ethernet.hpp>
#include <drivers/net/rtl8169/rtl8169.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::system::net;

namespace kernel::drivers::net::rtl8169 {

bool initialised = false;
vector<RTL8169*> devices;

static void RTL8169_Handler(registers_t *regs, uint64_t i)
{
    if (i >= devices.size()) return;
    RTL8169 *device = devices[i];

    uint16_t status = device->status();
    if (status & IST_SYSTEM_ERROR) error("RTL8169: Card #%zu: Error!", i);
    if (status & IST_TIME_OUT) if (device->debug) warn("RTL8169: Card #%zu: Time out!", i);
    if (status & IST_SOFT_INT) if (device->debug) warn("RTL8169: Card #%zu: Software interrupt!", i);
    if (status & IST_TX_UNAVAIL) if (device->debug) error("RTL8169: Card #%zu: TX descriptor unavailable!", i);
    if (status & IST_RX_FIFO_OVER) if (device->debug) error("RTL8169: Card #%zu: RX FIFO overflow!", i);
    if (status & IST_LINK_CHANGE) if (device->debug) log("RTL8169: Card #%zu: Link status change!", i);
    if (status & IST_RX_UNAVAIL) if (device->debug) error("RTL8169: Card #%zu: RX descriptor unavailable!", i);
    if (status & IST_TRANSMIT_ERR) if (device->debug) error("RTL8169: Card #%zu: Error while sending packet!", i);
    if (status & IST_TRANSMIT_OK) if (device->debug) log("RTL8169: Card #%zu: Packet sent!", i);
    if (status & IST_RECEIVE_ERR) if (device->debug) error("RTL8169: Card #%zu: Error while receiving packet!", i);
    if (status & IST_RECEIVE_OK)
    {
        if (device->debug) log("RTL8169: Card #%zu: Packet received!", i);
        device->receive();
    }
    device->irq_reset(status);
}

void RTL8169::outb(uint16_t addr, uint8_t val)
{
    if (this->BARType == 0) mmoutb(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else outb(this->IOBase + addr, val);
}
void RTL8169::outw(uint16_t addr, uint16_t val)
{
    if (this->BARType == 0) mmoutw(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else outw(this->IOBase + addr, val);
}
void RTL8169::outl(uint16_t addr, uint32_t val)
{
    if (this->BARType == 0) mmoutl(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else outl(this->IOBase + addr, val);
}
uint8_t RTL8169::inb(uint16_t addr)
{
    if (this->BARType == 0) return mminb(reinterpret_cast<void*>(this->MEMBase + addr));
    else return inb(this->IOBase + addr);
}
uint16_t RTL8169::inw(uint16_t addr)
{
    if (this->BARType == 0) return mminw(reinterpret_cast<void*>(this->MEMBase + addr));
    else return inw(this->IOBase + addr);
}
uint32_t RTL8169::inl(uint16_t addr)
{
    if (this->BARType == 0) return mminl(reinterpret_cast<void*>(this->MEMBase + addr));
    else return inl(this->IOBase + addr);
}

uint16_t RTL8169::status()
{
    return this->inw(REG_ISR);
}

void RTL8169::irq_reset(uint16_t status)
{
    this->outw(REG_ISR, status);
}

void RTL8169::read_mac()
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

void RTL8169::send(void *data, uint64_t length)
{
    lockit(this->lock);

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
    this->outb(REG_TPPOLL, 0x40);
    free(tdata);
}

void RTL8169::receive()
{
    for (size_t i = 0; (this->inb(REG_CR) & 0x01) == 0; i++)
    {
        if (this->debug) log("RTL8169: Handling packet #%zu!", i);

        // size_t length = this->rxdescs[this->rxcurr]->command & 0xFFFF;
        size_t length = (this->rxdescs[this->rxcurr]->command & 0x1FFF) - 4;
        uint64_t t = this->rxdescs[this->rxcurr]->buffer;
        void *packet = malloc(length);
        memcpy(packet, reinterpret_cast<void*>(t), length);

        this->rxdescs[this->rxcurr]->command |= RTL8169_OWN;
        if (++this->rxcurr >= RTL8169_NUM_RX_DESC) this->rxcurr = 0;

        ethernet::receive(this, reinterpret_cast<ethernet::ethHdr*>(packet));
        free(packet);
    }
}

void RTL8169::rxinit()
{
    Desc *descs = new Desc[RTL8169_NUM_RX_DESC];
    for (size_t i = 0; i < RTL8169_NUM_RX_DESC; i++)
    {
        this->rxdescs[i] = reinterpret_cast<Desc*>(reinterpret_cast<uint8_t*>(descs) + i * 16);
        if (i == RTL8169_NUM_RX_DESC - 1) this->rxdescs[i]->command = RTL8169_OWN | RTL8169_EOR | (RTL8169_RX_BUFF_SIZE & 0x3FFF);
        else this->rxdescs[i]->command = RTL8169_OWN | (RTL8169_RX_BUFF_SIZE & 0x3FFF);
        this->rxdescs[i]->buffer = malloc<uint64_t>(RTL8169_RX_BUFF_SIZE + 16);
    }
}

void RTL8169::txinit()
{
    Desc *descs = new Desc[RTL8169_NUM_TX_DESC];
    for (size_t i = 0; i < RTL8169_NUM_TX_DESC; i++)
    {
        this->txdescs[i] = reinterpret_cast<Desc*>(reinterpret_cast<uint8_t*>(descs) + i * 16);
        if (i == RTL8169_NUM_TX_DESC - 1) this->rxdescs[i]->command = RTL8169_EOR;
    }
}

void RTL8169::reset()
{
    this->outb(REG_CR, 0x10);
    while ((this->inb(REG_CR) & 0x10));
}

void RTL8169::start()
{
    this->outb(REG_CONFIG1, 0x01);
    this->reset();

    this->rxinit();
    this->txinit();

    this->outb(REG_9346CR, 0xC0);
    this->outl(REG_RCR, 0x0000E70F);
    this->outb(REG_CR, 0x04);
    this->outl(REG_TCR, 0x03000700);
    this->outw(REG_RMS, 0x1FFF);
    this->outb(REG_ETTHR, 0x3B);
    this->outl(REG_TNPDS_1, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->txdescs[0])));
    this->outl(REG_TNPDS_2, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->txdescs[0]) >> 32));
    this->outl(REG_RDSAR_1, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->rxdescs[0])));
    this->outl(REG_RDSAR_2, static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->rxdescs[0]) >> 32));
    this->outw(REG_IMR, 0xC1FF);
    this->outb(REG_CR, 0x0C);
    this->outb(REG_9346CR, 0x00);

    this->read_mac();
}

RTL8169::RTL8169(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering card #%zu", devices.size());

    pci::pcibar bar0 = pcidevice->get_bar(0);
    pci::pcibar bar2 = pcidevice->get_bar(2);
    this->BARType = bar2.mmio ? 0x00 : 0x01;
    this->MEMBase = bar2.address;
    this->IOBase = bar0.address;

    pcidevice->command(pci::CMD_BUS_MAST | pci::CMD_IO_SPACE | pci::CMD_MEM_SPACE, true);

    this->start();

    pcidevice->irq_set(RTL8169_Handler, devices.size());
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
        if (devices.back()->initialised == false)
        {
            free(devices.back());
            devices.pop_back();
            error("Could not initialise RTL8139 driver #%zu", devices.size());
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