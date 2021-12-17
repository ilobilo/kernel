// Copyright (C) 2021  ilobilo

#pragma once

#include <system/pci/pcidesc.hpp>
#include <lib/vector.hpp>
#include <stdint.h>

namespace kernel::system::pci {

struct pcidevice_t
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

struct translatedpcidevice_t
{
    pcidevice_t *device;
    const char *vendorstr;
    const char *devicestr;
    const char *progifstr;
    const char *subclassStr;
    const char *ClassStr;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
};

struct pciheader0
{
    pcidevice_t device;
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
    pcidevice_t device;
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
    pcidevice_t device;
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

extern vector<translatedpcidevice_t*> pcidevices;

uint8_t readb(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset);
uint16_t readw(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset);
uint32_t readl(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset);

void writew(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint16_t value);
void writeb(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint8_t value);
void writel(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint32_t value);

translatedpcidevice_t *search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip);
translatedpcidevice_t *search(uint16_t vendor, uint16_t device, int skip);

size_t count(uint8_t Class, uint8_t subclass, uint8_t progif);
size_t count(uint16_t vendor, uint16_t device);

void init();
}