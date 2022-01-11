// Copyright (C) 2021  ilobilo

#include <drivers/net/stack/ethernet/ethernet.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/memory.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::net::stack;
using namespace kernel::system::cpu;

namespace kernel::drivers::net::e1000 {

bool initialised = false;
vector<E1000*> devices;

static void E1000_Handler(registers_t *regs)
{
    for (size_t i = 0; i < devices.size(); i++)
    {
        E1000 *device = devices[i];
        device->irq_reset();
        uint32_t status = device->status();
        if (status & (1 << 24)) if (device->debug) warn("E1000: Card #%zu: Time out!", i);
        if (status & (1 << 23)) if (device->debug) warn("E1000: Card #%zu: Non-fatal error!", i);
        if (status & (1 << 22)) error("E1000: Card #%zu: Fatal error!", i);
        if (status & (1 << 7))
        {
            if (device->debug) log("E1000: Card #%zu: Packets received!", i);
            device->receive();
        }
        if (status & (1 << 2))
        {
            if (device->debug) log("E1000: Card #%zu: Link status changed!", i);
            device->startlink();
        }
        if (status & (1 << 0)) if (device->debug) log("E1000: Card #%zu: Packet sent!", i);
    }
}

uint32_t E1000::status()
{
    return this->incmd(0xC0);
}

void E1000::irq_reset()
{
    this->outcmd(REG_IMASK, 0x01);
}

void E1000::outcmd(uint16_t addr, uint32_t val)
{
    if (this->BARType == 0) mmoutl(reinterpret_cast<void*>(this->MEMBase + addr), val);
    else
    {
        outl(this->IOBase, addr);
        outl(this->IOBase + 4, addr);
    }
}

uint32_t E1000::incmd(uint16_t addr)
{
    if (this->BARType == 0) return mminl(reinterpret_cast<void*>(this->MEMBase + addr));
    else
    {
        outl(this->IOBase, addr);
        return inl(this->IOBase + 4);
    }
}

bool E1000::detecteeprom()
{
    this->outcmd(REG_EEPROM, 0x01);
    for (size_t i = 0; i < 1000 && !this->eeprom; i++)
    {
        if (this->incmd(REG_EEPROM) & 0x10)
        {
            this->eeprom = true;
            break;
        }
        else this->eeprom = false;
    }
    return this->eeprom;
}

uint32_t E1000::readeeprom(uint8_t addr)
{
    uint32_t tmp = 0;
    if (this->eeprom)
    {
        this->outcmd(REG_EEPROM, 1 | (static_cast<uint32_t>(addr) << 8));
        while (!((tmp = this->incmd(REG_EEPROM)) & (1 << 4)));
    }
    else
    {
        this->outcmd(REG_EEPROM, 1 | (static_cast<uint32_t>(addr) << 2));
        while (!((tmp = this->incmd(REG_EEPROM)) & (1 << 1)));
    }
    return static_cast<uint16_t>((tmp >> 16) & 0xFFFF);
}

void E1000::read_mac()
{
    if (this->eeprom)
    {
        uint32_t tmp = this->readeeprom(0);
        this->MAC[0] = tmp & 0xFF;
        this->MAC[1] = tmp >> 8;
        tmp = this->readeeprom(1);
        this->MAC[2] = tmp & 0xFF;
        this->MAC[3] = tmp >> 8;
        tmp = this->readeeprom(2);
        this->MAC[4] = tmp & 0xFF;
        this->MAC[5] = tmp >> 8;
    }
    else
    {
        this->MAC[0] = this->incmd(this->MEMBase + 0x5400);
        this->MAC[1] = this->incmd(this->MEMBase + 0x5401);
        this->MAC[2] = this->incmd(this->MEMBase + 0x5402);
        this->MAC[3] = this->incmd(this->MEMBase + 0x5403);
        this->MAC[4] = this->incmd(this->MEMBase + 0x5404);
        this->MAC[5] = this->incmd(this->MEMBase + 0x5405);
    }
    if (this->debug) log("MAC Address: %X:%X:%X:%X:%X:%X", this->MAC[0], this->MAC[1], this->MAC[2], this->MAC[3], this->MAC[4], this->MAC[5]);
}

void E1000::send(uint8_t *data, uint64_t length)
{
    acquire_lock(this->lock);
    void *tdata = malloc(length);
    memcpy(tdata, data, length);
    this->txdescs[this->txcurr]->addr = reinterpret_cast<uint64_t>(tdata);
    this->txdescs[this->txcurr]->length = length;
    this->txdescs[this->txcurr]->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    this->txdescs[this->txcurr]->status = 0;
    uint8_t old_cur = this->txcurr;
    this->txcurr = (this->txcurr + 1) % E1000_NUM_TX_DESC;
    this->outcmd(REG_TXDESCTAIL, this->txcurr);
    size_t timeout = 5000;
    while (!(this->txdescs[old_cur]->status & 0xFF))
    {
        timeout--;
        if (timeout <= 0)
        {
            if (this->debug) error("E1000: Could not send the packet!");
            break;
        }
    }
    free(tdata);
    release_lock(this->lock);
}

void E1000::receive()
{
    uint16_t old_cur = 0;
    for (size_t i = 0; this->rxdescs[this->rxcurr]->status & 0x01; i++)
    {
        if (this->debug) log("E1000: Handling packet #%zu!", i);

        uint16_t length = this->rxdescs[this->rxcurr]->length;
        uint8_t *packet = static_cast<uint8_t*>(malloc(length));
        memcpy(packet, reinterpret_cast<uint8_t*>(this->rxdescs[this->rxcurr]->addr), length);

        this->rxdescs[this->rxcurr]->status = 0;
        old_cur = this->rxcurr;
        this->rxcurr = (this->rxcurr + 1) % E1000_NUM_RX_DESC;
        this->outcmd(REG_RXDESCTAIL, old_cur);

        ethernet::receive(this, reinterpret_cast<ethernet::ethHdr*>(packet), length);
        free(packet);
    }
}

void E1000::rxinit()
{
    uint8_t *ptr = static_cast<uint8_t*>(malloc(sizeof(RXDesc) * E1000_NUM_RX_DESC + 16));
    RXDesc *descs = reinterpret_cast<RXDesc*>(ptr);
    for (size_t i = 0; i < E1000_NUM_RX_DESC; i++)
    {
        this->rxdescs[i] = reinterpret_cast<RXDesc*>(reinterpret_cast<uint8_t*>(descs) + i * 16);
        this->rxdescs[i]->addr = reinterpret_cast<uint64_t>(static_cast<uint8_t*>(malloc(E1000_RX_BUFF_SIZE + 16)));
        this->rxdescs[i]->status = 0;
    }
    this->outcmd(REG_RXDESCLO, reinterpret_cast<uint64_t>(ptr));
    this->outcmd(REG_RXDESCHI, reinterpret_cast<uint64_t>(ptr) >> 32);
    this->outcmd(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);
    this->outcmd(REG_RXDESCHEAD, 0);
    this->outcmd(REG_RXDESCTAIL, E1000_NUM_RX_DESC);
    this->rxcurr = 0;
    this->outcmd(REG_RCTRL, RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RCTL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_8192);
}

void E1000::txinit()
{
    uint8_t *ptr = static_cast<uint8_t*>(malloc(sizeof(TXDesc) * E1000_NUM_TX_DESC + 16));
    TXDesc *descs = reinterpret_cast<TXDesc*>(ptr);
    for (size_t i = 0; i < E1000_NUM_TX_DESC; i++)
    {
        this->txdescs[i] = reinterpret_cast<TXDesc*>(reinterpret_cast<uint8_t*>(descs) + i * 16);
        this->txdescs[i]->addr = 0;
        this->txdescs[i]->cmd = 0;
        this->txdescs[i]->status = TSTA_DD;
    }
    this->outcmd(REG_TXDESCLO, static_cast<uint32_t>(reinterpret_cast<uint64_t>(ptr)));
    this->outcmd(REG_TXDESCHI, static_cast<uint32_t>(reinterpret_cast<uint64_t>(ptr) >> 32));
    this->outcmd(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);
    this->outcmd(REG_TXDESCHEAD, 0);
    this->outcmd(REG_TXDESCTAIL, 0);
    this->txcurr = 0;
    this->outcmd(REG_TCTRL, TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT) | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);
    this->outcmd(REG_TCTRL, 0b0110000000000111111000011111010);
    this->outcmd(REG_TIPG, 0x0060200A);
}

void E1000::intenable()
{
    this->outcmd(REG_IMASK, 0x1F6DC);
    this->outcmd(REG_IMASK, 0xFF & ~4);
    this->incmd(0xC0);
}

void E1000::startlink()
{
    this->outcmd(REG_CTRL, this->incmd(REG_CTRL) | ECTRL_SLU);
}

void E1000::start()
{
    acquire_lock(this->lock);
    this->detecteeprom();
    this->read_mac();
    this->startlink();
    for (size_t i = 0; i < 0x80; i++) this->outcmd(0x5200 + i * 4, 0);

    this->intenable();
    this->rxinit();
    this->txinit();
    release_lock(this->lock);
}

E1000::E1000(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering card #%zu", devices.size());

    uint32_t BAR0 = 0;
    if (pci::legacy) BAR0 = pcidevice->readl(pci::PCI_BAR0);
    else BAR0 = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->BAR[0];

    this->BARType = BAR0 & 0x01;
    this->IOBase = pcidevice->get_bar(PCI_BAR_IO) & ~1;
    this->MEMBase = pcidevice->get_bar(PCI_BAR_MEM) & ~3;

    pcidevice->command(pci::CMD_BUS_MAST | pci::CMD_IO_SPACE, true);

    this->start();

    pcidevice->irq_set(E1000_Handler);
}

uint16_t ids[94][2] = {
    { 0x8086, 0x1000 },
    { 0x8086, 0x1001 },
    { 0x8086, 0x1004 },
    { 0x8086, 0x1008 },
    { 0x8086, 0x1009 },
    { 0x8086, 0x100C },
    { 0x8086, 0x100D },
    { 0x8086, 0x100E },
    { 0x8086, 0x100F },
    { 0x8086, 0x1010 },
    { 0x8086, 0x1011 },
    { 0x8086, 0x1012 },
    { 0x8086, 0x1013 },
    { 0x8086, 0x1014 },
    { 0x8086, 0x1015 },
    { 0x8086, 0x1016 },
    { 0x8086, 0x1017 },
    { 0x8086, 0x1018 },
    { 0x8086, 0x1019 },
    { 0x8086, 0x101A },
    { 0x8086, 0x101D },
    { 0x8086, 0x101E },
    { 0x8086, 0x1026 },
    { 0x8086, 0x1027 },
    { 0x8086, 0x1028 },
    { 0x8086, 0x1049 },
    { 0x8086, 0x104A },
    { 0x8086, 0x104B },
    { 0x8086, 0x104C },
    { 0x8086, 0x104D },
    { 0x8086, 0x105E },
    { 0x8086, 0x105F },
    { 0x8086, 0x1060 },
    { 0x8086, 0x1075 },
    { 0x8086, 0x1076 },
    { 0x8086, 0x1077 },
    { 0x8086, 0x1078 },
    { 0x8086, 0x1079 },
    { 0x8086, 0x107A },
    { 0x8086, 0x107B },
    { 0x8086, 0x107C },
    { 0x8086, 0x107D },
    { 0x8086, 0x107E },
    { 0x8086, 0x107F },
    { 0x8086, 0x108A },
    { 0x8086, 0x108B },
    { 0x8086, 0x108C },
    { 0x8086, 0x1096 },
    { 0x8086, 0x1098 },
    { 0x8086, 0x1099 },
    { 0x8086, 0x109A },
    { 0x8086, 0x10A4 },
    { 0x8086, 0x10A7 },
    { 0x8086, 0x10A9 },
    { 0x8086, 0x10B5 },
    { 0x8086, 0x10B9 },
    { 0x8086, 0x10BA },
    { 0x8086, 0x10BB },
    { 0x8086, 0x10BC },
    { 0x8086, 0x10BD },
    { 0x8086, 0x10C4 },
    { 0x8086, 0x10C5 },
    { 0x8086, 0x10C9 },
    { 0x8086, 0x10D3 },
    { 0x8086, 0x10A9 },
    { 0x8086, 0x10CB },
    { 0x8086, 0x10E5 },
    { 0x8086, 0x10EA },
    { 0x8086, 0x10EB },
    { 0x8086, 0x10EF },
    { 0x8086, 0x10F0 },
    { 0x8086, 0x10F5 },
    { 0x8086, 0x1502 },
    { 0x8086, 0x1503 },
    { 0x8086, 0x150A },
    { 0x8086, 0x150E },
    { 0x8086, 0x1521 },
    { 0x8086, 0x1533 },
    { 0x8086, 0x157B },
    { 0x8086, 0x153A },
    { 0x8086, 0x153B },
    { 0x8086, 0x1559 },
    { 0x8086, 0x155A },
    { 0x8086, 0x15A0 },
    { 0x8086, 0x15A1 },
    { 0x8086, 0x15A2 },
    { 0x8086, 0x15A3 },
    { 0x8086, 0x156F },
    { 0x8086, 0x1570 },
    { 0x8086, 0x15B7 },
    { 0x8086, 0x15B8 },
    { 0x8086, 0x15BB },
    { 0x8086, 0x15D7 },
    { 0x8086, 0x15E3 }
};

bool search(uint16_t vendorid, uint16_t deviceid)
{
    size_t count = pci::count(vendorid, deviceid);
    if (count == 0) return false;

    for (size_t i = 0; i < count; i++)
    {
        devices.push_back(new E1000(pci::search(vendorid, deviceid, i)));
    }
    return true;
}

void init()
{
    log("Initialising E1000 driver");

    if (initialised)
    {
        warn("E1000 driver has already been initialised!\n");
        return;
    }

    bool found[94] = { [0 ... 93] = false };
    for (size_t i = 0; i < 94; i++)
    {
        found[i] = search(ids[i][0], ids[i][1]);
    }
    for (size_t i = 0; i < 94; i++)
    {
        if (found[i] == true)
        {
            serial::newline();
            initialised = true;
            return;
        }
    }
    error("No E1000 cards found!\n");
}
}