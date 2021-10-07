#pragma once

#include <system/pci/pcidesc.hpp>
#include <stdint.h>

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

extern pcideviceheader *pcidevices;
extern uint64_t pcidevcount;

void PCI_init();
