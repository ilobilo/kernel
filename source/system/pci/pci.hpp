// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/cpu/idt/idt.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/vector.hpp>
#include <lib/string.hpp>
#include <lib/mmio.hpp>
#include <lib/io.hpp>
#include <cstdint>

namespace kernel::system::pci {

enum offsets
{
    PCI_VENDOR_ID = 0x00,
    PCI_DEVICE_ID = 0x02,
    PCI_COMMAND = 0x04,
    PCI_STATUS = 0x06,
    PCI_REVISION_ID = 0x08,
    PCI_PROG_IF = 0x09,
    PCI_SUBCLASS = 0x0A,
    PCI_CLASS = 0x0B,
    PCI_CACHE_LINE_SIZE = 0x0C,
    PCI_LATENCY_TIMER = 0x0D,
    PCI_HEADER_TYPE = 0x0E,
    PCI_BIST = 0x0F,
    PCI_BAR0 = 0x10,
    PCI_BAR1 = 0x14,
    PCI_BAR2 = 0x18,
    PCI_BAR3 = 0x1C,
    PCI_BAR4 = 0x20,
    PCI_BAR5 = 0x24,
    PCI_CAPABPTR = 0x34,
    PCI_INTERRUPT_LINE = 0x3C,
    PCI_INTERRUPT_PIN = 0x3D
};

enum cmds
{
    CMD_IO_SPACE = (1 << 0),
    CMD_MEM_SPACE = (1 << 1),
    CMD_BUS_MAST = (1 << 2),
    CMD_SPEC_CYC = (1 << 3),
    CMD_MEM_WRITE = (1 << 4),
    CMD_VGA_PS = (1 << 5),
    CMD_PAR_ERR = (1 << 6),
    CMD_SERR = (1 << 8),
    CMD_FAST_B2B = (1 << 9),
    CMD_INT_DIS = (1 << 10),
};

struct pcibar
{
    uint64_t address;
    bool mmio;
    bool prefetchable;
};

struct pciheader_t
{
    uint16_t vendorid;
    uint16_t deviceid;
    uint16_t command;
    uint16_t status;
    uint8_t revisionid;
    uint8_t progif;
    uint8_t subclass;
    uint8_t Class;
    uint8_t cachelinesize;
    uint8_t latencytimer;
    uint8_t headertype;
    uint8_t bist;
};

extern bool legacy;
static inline void *get_addr(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    if (legacy)
    {
        uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3)) | 0x80000000;
        outl(0xcf8, address);
    }
    else
    {
        for (size_t i = 0; i < ((acpi::mcfghdr->header.length) - sizeof(acpi::MCFGHeader)) / sizeof(acpi::MCFGEntry); i++)
        {
            acpi::MCFGEntry &entry = acpi::mcfghdr->entries[i];
            if (entry.segment == seg)
            {
                if ((bus >= entry.startbus) && (bus <= entry.endbus))
                {
                    return reinterpret_cast<void*>((entry.baseaddr + (((bus - entry.startbus) << 20) | (dev << 15) | (func << 12))) + offset);
                }
            }
        }
    }
    return nullptr;
}

static inline uint8_t readb(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    if (legacy)
    {
        get_addr(seg, bus, dev, func, offset);
        return inb(0xCFC + (offset & 3));
    }
    return mminb(get_addr(seg, bus, dev, func, offset));
}
static inline uint16_t readw(uint16_t seg, int8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    if (legacy)
    {
        get_addr(seg, bus, dev, func, offset);
        return inw(0xCFC + (offset & 3));
    }
    return mminw(get_addr(seg, bus, dev, func, offset));
}
static inline uint32_t readl(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    if (legacy)
    {
        get_addr(seg, bus, dev, func, offset);
        return inl(0xCFC + (offset & 3));
    }
    return mminl(get_addr(seg, bus, dev, func, offset));
}

static inline void writeb(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint8_t value)
{
    if (legacy)
    {
        get_addr(seg, bus, dev, func, offset);
        outb(0xCFC + (offset & 3), value);
    }
    else mmoutb(get_addr(seg, bus, dev, func, offset), value);
}
static inline void writew(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint16_t value)
{
    if (legacy)
    {
        get_addr(seg, bus, dev, func, offset);
        outw(0xCFC + (offset & 3), value);
    }
    else mmoutw(get_addr(seg, bus, dev, func, offset), value);
}
static inline void writel(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint32_t value)
{
    if (legacy)
    {
        get_addr(seg, bus, dev, func, offset);
        outl(0xCFC + (offset & 3), value);
    }
    else mmoutl(get_addr(seg, bus, dev, func, offset), value);
}

extern bool legacy;
struct pcidevice_t
{
    pciheader_t *device;
    std::string vendorstr;
    std::string devicestr;
    std::string progifstr;
    std::string subclassStr;
    std::string ClassStr;
    uint16_t seg;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;

    bool int_on;

    bool msi_support;
    uint16_t msi_offset;

    void *get_addr(uint32_t offset)
    {
        return kernel::system::pci::get_addr(this->seg, this->bus, this->dev, this->func, offset);
    }

    uint8_t readb(uint32_t offset)
    {
        return kernel::system::pci::readb(this->seg, this->bus, this->dev, this->func, offset);
    }
    uint16_t readw(uint32_t offset)
    {
        return kernel::system::pci::readw(this->seg, this->bus, this->dev, this->func, offset);
    }
    uint32_t readl(uint32_t offset)
    {
        return kernel::system::pci::readl(this->seg, this->bus, this->dev, this->func, offset);
    }

    void writeb(uint32_t offset, uint8_t value)
    {
        kernel::system::pci::writeb(this->seg, this->bus, this->dev, this->func, offset, value);
    }
    void writew(uint32_t offset, uint16_t value)
    {
        kernel::system::pci::writew(this->seg, this->bus, this->dev, this->func, offset, value);
    }
    void writel(uint32_t offset, uint32_t value)
    {
        kernel::system::pci::writel(this->seg, this->bus, this->dev, this->func, offset, value);
    }

    void command(uint64_t cmd, bool enable)
    {
        if (legacy)
        {
            uint32_t command = this->device->command;
            if (enable) command |= cmd;
            else command &= ~cmd;
            this->writew(PCI_COMMAND, command);
            this->device->command = this->readw(PCI_COMMAND);
        }
        else
        {
            if (enable) this->device->command |= cmd;
            else this->device->command &= ~cmd;
        }
    }

    pcibar get_bar(size_t bar);
    void msi_set(uint8_t vector);
    uint8_t irq_set(cpu::idt::int_handler_func handler);
    uint8_t irq_set(cpu::idt::int_handler_func_arg handler, uint64_t args);
};

struct pciheader0
{
    pciheader_t device;
    uint32_t BAR[6];
    uint32_t cardbusCisPtr;
    uint16_t subsysVendorID;
    uint16_t subsysID;
    uint32_t expRomBaseAddr;
    uint8_t capabPtr;
    uint8_t rsv0;
    uint16_t rsv1;
    uint32_t rsv2;
    uint8_t intLine;
    uint8_t intPin;
    uint8_t minGrid;
    uint8_t maxLatency;
};

// PCI-to-PCI bridge
struct pciheader1
{
    pciheader_t device;
    uint32_t BAR[2];
    uint8_t primBus;
    uint8_t secBus;
    uint8_t subBus;
    uint8_t secLatTimer;
    uint8_t iobase;
    uint8_t iolimit;
    uint16_t secStatus;
    uint16_t memBase;
    uint16_t memLimit;
    uint16_t prefMembase;
    uint16_t prefMemlimit;
    uint32_t prefBaseUp;
    uint32_t prefLimitUp;
    uint16_t ioBaseUp;
    uint16_t ioLimitUp;
    uint8_t capabPtr;
    uint8_t rsv0;
    uint16_t rsv1;
    uint32_t expRomBase;
    uint8_t intLine;
    uint8_t intPin;
    uint16_t bridgeCtrl;
};

// PCI-to-CardBus bridge
struct pciheader2
{
    pciheader_t device;
    uint32_t cbusSocketBase;
    uint8_t capListOffset;
    uint8_t rsv0;
    uint16_t secStatus;
    uint8_t pciBusNum;
    uint8_t cbusNum;
    uint8_t subBusNum;
    uint8_t cbusLatTimer;
    uint32_t memBase0;
    uint32_t memLimit0;
    uint32_t memBase1;
    uint32_t memLimit1;
    uint32_t ioBase0;
    uint32_t ioLimit0;
    uint32_t ioBase1;
    uint32_t ioLimit1;
    uint8_t intLine;
    uint8_t intPin;
    uint16_t bridgeCtrl;
    uint16_t subDevID;
    uint16_t subVenID;
    uint32_t pcLegModeBase;
};

extern bool initialised;

extern vector<pcidevice_t*> devices;

pcidevice_t *search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip);
pcidevice_t *search(uint16_t vendor, uint16_t device, int skip);

size_t count(uint8_t Class, uint8_t subclass, uint8_t progif);
size_t count(uint16_t vendor, uint16_t device);

void init();
}