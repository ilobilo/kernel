// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <lib/io.hpp>
#include <stdint.h>

namespace kernel::system::pci {

#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0A
#define PCI_CLASS 0x0B
#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER 0x0D
#define PCI_HEADER_TYPE 0x0E
#define PCI_BIST 0x0f
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D

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

static inline void get_addr(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3)) | 0x80000000;
    outl(0xcf8, address);
}

static inline uint8_t readb(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    get_addr(bus, dev, func, offset);
    return inb(0xCFC + (offset & 3));
}
static inline uint16_t readw(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    get_addr(bus, dev, func, offset);
    return inw(0xCFC + (offset & 3));
}
static inline uint32_t readl(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    get_addr(bus, dev, func, offset);
    return inl(0xCFC + (offset & 3));
}

static inline void writeb(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint8_t value)
{
    get_addr(bus, dev, func, offset);
    outb(0xCFC + (offset & 3), value);
}
static inline void writew(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint16_t value)
{
    get_addr(bus, dev, func, offset);
    outw(0xCFC + (offset & 3), value);
}
static inline void writel(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint32_t value)
{
    get_addr(bus, dev, func, offset);
    outl(0xCFC + (offset & 3), value);
}

extern bool legacy;
struct pcidevice_t
{
    pciheader_t *device;
    const char *vendorstr;
    const char *devicestr;
    const char *progifstr;
    const char *subclassStr;
    const char *ClassStr;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;

    void get_addr(uint32_t offset)
    {
        kernel::system::pci::get_addr(this->bus, this->dev, this->func, offset);
    }

    uint8_t readb(uint32_t offset)
    {
        return kernel::system::pci::readb(this->bus, this->dev, this->func, offset);
    }
    uint16_t readw(uint32_t offset)
    {
        return kernel::system::pci::readw(this->bus, this->dev, this->func, offset);
    }
    uint32_t readl(uint32_t offset)
    {
        return kernel::system::pci::readl(this->bus, this->dev, this->func, offset);
    }

    void writeb(uint32_t offset, uint8_t value)
    {
        kernel::system::pci::writeb(this->bus, this->dev, this->func, offset, value);
    }
    void writew(uint32_t offset, uint16_t value)
    {
        kernel::system::pci::writew(this->bus, this->dev, this->func, offset, value);
    }
    void writel(uint32_t offset, uint32_t value)
    {
        kernel::system::pci::writel(this->bus, this->dev, this->func, offset, value);
    }

    void bus_mastering(bool enable)
    {
        uint32_t command = this->device->command;
        if(!(command & (1 << 2)))
        {
            if (enable) command |= (1 << 2);
            else command &= ~(1 << 2);
            this->writew(PCI_COMMAND, command);
            if (legacy) this->device->command = this->readw(PCI_COMMAND);
        }
    }
};

struct pciheader0
{
    pciheader_t device;
    uint32_t BAR0;
    uint32_t BAR1;
    uint32_t BAR2;
    uint32_t BAR3;
    uint32_t BAR4;
    uint32_t BAR5;
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
    uint32_t BAR0;
    uint32_t BAR1;
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
extern bool legacy;

extern vector<pcidevice_t*> pcidevices;

pcidevice_t *search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip);
pcidevice_t *search(uint16_t vendor, uint16_t device, int skip);

size_t count(uint8_t Class, uint8_t subclass, uint8_t progif);
size_t count(uint16_t vendor, uint16_t device);

void init();
}