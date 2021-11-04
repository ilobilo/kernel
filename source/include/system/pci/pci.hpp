// Copyright (C) 2021  ilobilo

#pragma once

#include <system/pci/pcidesc.hpp>
#include <stdint.h>

namespace kernel::system::pci {

struct pcideviceheader
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

struct translatedpcideviceheader
{
    uint16_t vendorid;
    uint16_t deviceid;
    char *vendorstr;
    char *devicestr;
    uint16_t command;
    uint16_t status;
    uint8_t revisionid;
    uint8_t progif;
    char *progifstr;
    uint8_t subclass;
    char *subclassStr;
    uint8_t Class;
    char *ClassStr;
    uint8_t cachelinesize;
    uint8_t latencytimer;
    uint8_t headertype;
    uint8_t bist;
};

struct pciheader0
{
    pcideviceheader header;
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

extern bool initialised;
extern bool legacy;
extern bool use_pciids;

extern translatedpcideviceheader **pcidevices;
extern uint64_t pcidevcount;

translatedpcideviceheader *search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip);

void init();
}