// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <stdint.h>

using namespace kernel::drivers::display;

namespace kernel::drivers::block::drivemgr {

enum interface
{
    NVME,
    SATA,
    IDE,
    FDC
};

enum type
{
    ATA,
    ATAPI
};

class Drive
{
    public:
    char name[32] = "New drive";
    uint8_t *buffer = nullptr;
    interface interface;
    type type;

    virtual bool read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
    {
        serial::err("Read function for this device is not available!");
        return false;
    }

    virtual bool write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
    {
        serial::err("Write function for this device is not available!");
        return false;
    }
};

extern bool initialised;
extern vector<Drive*> drives;

void init();
}