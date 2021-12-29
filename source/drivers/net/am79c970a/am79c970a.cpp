// Copyright (C) 2021  ilobilo

#include <drivers/net/am79c970a/am79c970a.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/memory.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>

using namespace kernel::system::cpu;

namespace kernel::drivers::net::am79c970a {

bool initialised = false;
static bool first = true;
vector<AM79C970A*> devices;

static void AM79C970A_Handler(registers_t *regs)
{
    for (size_t i = 0; i < devices.size(); i++)
    {
        AM79C970A *device = devices[i];
        uint32_t status = device->status();

        if (status & (1 << 8)) log("AM79C970A: Card #%zu: Init done!", i);
        if (status & (1 << 9)) log("AM79C970A: Card #%zu: Packet sent!", i);
        if (status & (1 << 10))
        {
            device->recive();
            log("AM79C970A: Card #%zu: Packet recived!", i);
        }
        if (status & (1 << 12)) error("AM79C970A: Card #%zu: Missed frame!", i);
        if (status & (1 << 13)) error("AM79C970A: Card #%zu: Collision error!", i);
        if (status & (1 << 15)) error("AM79C970A: Card #%zu: error!", i);

        device->irq_reset(status);
    }
}

void AM79C970A::outrap32(uint32_t val)
{
    outl(this->IOBase + 0x14, val);
}

uint32_t AM79C970A::incsr32(uint32_t csr_no)
{
    this->outrap32(csr_no);
    return inl(this->IOBase + 0x10);
}

void AM79C970A::outcsr32(uint32_t csr_no, uint32_t val)
{
    this->outrap32(csr_no);
    outl(this->IOBase + 0x10, val);
}

uint32_t AM79C970A::inbcr32(uint32_t bcr_no)
{
    this->outrap32(bcr_no);
    return inl(this->IOBase + 0x1C);
}

void AM79C970A::outbcr32(uint32_t bcr_no, uint32_t val)
{
    this->outrap32(bcr_no);
    outl(this->IOBase + 0x1C, val);
}

void AM79C970A::read_mac()
{
    uint32_t mac1 = inl(IOBase + 0x00);
    uint32_t mac2 = inl(IOBase + 0x04);

    this->MAC[0] = mac1;
    this->MAC[1] = mac1 >> 8;
    this->MAC[2] = mac1 >> 16;
    this->MAC[3] = mac1 >> 24;
    this->MAC[4] = mac2;
    this->MAC[5] = mac2 >> 8;

    log("MAC Address: %X:%X:%X:%X:%X:%X", this->MAC[0], this->MAC[1], this->MAC[2], this->MAC[3], this->MAC[4],this-> MAC[5]);
}

void AM79C970A::send(void *data, uint64_t length)
{
    acquire_lock(this->lock);
    if (!this->owner(this->txdesc, this->txcurr))
    {
        this->outcsr32(0, 1 << 3);
        return;
    }
    memcpy(reinterpret_cast<void*>(this->txbuffers + BUFF_LEN * this->txcurr), data, length);
    this->txdesc[DESC_LEN * this->txcurr + 7] |= 0x02;
    this->txdesc[DESC_LEN * this->txcurr + 7] |= 0x01;
    uint16_t bcnt = (static_cast<uint16_t>(-length) & 0x0FFF) | 0xF000;
    *reinterpret_cast<uint16_t*>(&this->txdesc[DESC_LEN * this->txcurr + 4]) = bcnt;
    this->txdesc[DESC_LEN * this->txcurr + 7] |= (1 << 7);
    this->txcurr = this->txnext();
    release_lock(this->lock);
}

void AM79C970A::recive()
{
    acquire_lock(this->lock);
    while (this->owner(this->rxdesc, this->rxcurr))
    {
        // Handle packet
        // uint16_t length = *reinterpret_cast<uint16_t*>(&this->rxdesc[DESC_LEN * this->rxcurr + 8]);
        // void *packet = reinterpret_cast<void*>(this->rxbuffers + BUFF_LEN * this->rxcurr);

        this->rxdesc[this->rxcurr * DESC_LEN + 7] = (1 << 7);
        this->rxcurr = this->rxnext();
    }
    release_lock(this->lock);
}

uint16_t AM79C970A::status()
{
    return this->incsr32(0);
}

void AM79C970A::irq_reset(uint32_t status)
{
    this->outcsr32(0, this->incsr32(0) | status);
}

void AM79C970A::reset()
{
    inl(this->IOBase + 0x18);
    inw(this->IOBase + 0x14);
    outl(this->IOBase + 0x10, 0);
    this->outcsr32(58, (this->incsr32(58) & 0xFF00) | 0x02);
    this->outbcr32(2, this->inbcr32(2) | 0x02);
}

uint16_t AM79C970A::rxnext()
{
    uint16_t ret = this->rxcurr + 1;
    if (ret == AM79C970A_NUM_RX_DESC) ret = 0;
    return ret;
}

uint16_t AM79C970A::txnext()
{
    uint16_t ret = this->txcurr + 1;
    if (ret == AM79C970A_NUM_TX_DESC) ret = 0;
    return ret;
}

int AM79C970A::owner(uint8_t *desc, size_t i)
{
    return !(desc[DESC_LEN * i + 7] & (1 << 7));
}

void AM79C970A::initDE(size_t i, bool rx)
{
    uint8_t *desc = (rx ? this->rxdesc : this->txdesc);
    uint32_t address = static_cast<uint32_t>(rx ? reinterpret_cast<uint64_t>(&this->rxbuffers[i]) : reinterpret_cast<uint64_t>(&this->txbuffers[i]));

    *reinterpret_cast<uint32_t*>(&desc[DESC_LEN * i]) = address + BUFF_LEN * i;
    uint16_t bcnt = (static_cast<uint16_t>(-BUFF_LEN) & 0x0FFF) | 0xF000;
    *reinterpret_cast<uint16_t*>(&desc[DESC_LEN * i + 4]) = bcnt;
    if (rx) desc[DESC_LEN * i + 7] = (1 << 7);
}

void AM79C970A::start()
{
    acquire_lock(this->lock);
    this->reset();
    this->read_mac();

    this->initblock = new initBlock;

    this->initblock->mode = 0x00;
    this->initblock->rxdescs = AM79C970A_LOG2_NUM_RX_DESC;
    this->initblock->txdescs = AM79C970A_LOG2_NUM_TX_DESC;
    memcpy(this->initblock->mac, this->MAC, 6);
    this->initblock->logaddr = 0x00;

    this->rxdesc = static_cast<uint8_t*>(calloc(AM79C970A_NUM_RX_DESC, DESC_LEN));
    this->txdesc = static_cast<uint8_t*>(calloc(AM79C970A_NUM_TX_DESC, DESC_LEN));

    for (size_t i = 0; i < AM79C970A_NUM_RX_DESC; i++)
    {
        this->rxbuffers[i] = static_cast<uint8_t*>(malloc(BUFF_LEN));
        this->initDE(i, true);
    }
    for (size_t i = 0; i < AM79C970A_NUM_TX_DESC; i++)
    {
        this->txbuffers[i] = static_cast<uint8_t*>(malloc(BUFF_LEN));
        this->initDE(i, false);
    }

    this->initblock->rxdescaddr = static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->rxdesc));
    this->initblock->txdescaddr = static_cast<uint32_t>(reinterpret_cast<uint64_t>(&this->txdesc));

    this->outcsr32(1, static_cast<uint32_t>(reinterpret_cast<uint64_t>(initblock)));
    this->outcsr32(2, static_cast<uint32_t>(reinterpret_cast<uint64_t>(initblock)) >> 16);

    this->outcsr32(0, (1 << 0));
    while (!(this->incsr32(0) & (1 << 8)));
    this->outcsr32(0, (1 << 1));
    release_lock(this->lock);
}

AM79C970A::AM79C970A(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering AM79C970A driver #%zu", devices.size());

    uint32_t BAR0 = 0;
    if (pci::legacy) BAR0 = pcidevice->readl(pci::PCI_BAR0);
    else BAR0 = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->BAR0;

    this->BARType = BAR0 & 0x01;
    this->IOBase = pcidevice->get_bar(PCI_BAR_IO) & ~1;

    pcidevice->command(pci::CMD_BUS_MAST | pci::CMD_IO_SPACE, true);

    uint8_t IRQ = 0;
    if (pci::legacy) IRQ = pcidevice->readl(pci::PCI_INTERRUPT_LINE);
    else IRQ = reinterpret_cast<pci::pciheader0*>(pcidevice->device)->intLine;

    if (first)
    {
        first = false;
        idt::register_interrupt_handler(IRQ + 32, AM79C970A_Handler);
    }

    this->start();
}

void init()
{
    log("Initialising AM79C970A driver");

    if (initialised)
    {
        warn("AM79C970A driver has already been initialised!\n");
        return;
    }

    size_t count = pci::count(0x1022, 0x2000);
    if (count == 0)
    {
        error("No AM79C970A cards found!\n");
        return;
    }

    for (size_t i = 0; i < count; i++)
    {
        devices.push_back(new AM79C970A(pci::search(0x1022, 0x2000, i)));
    }

    serial::newline();
    initialised = true;
}
}