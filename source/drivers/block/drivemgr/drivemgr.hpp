// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <lib/log.hpp>
#include <stdint.h>

namespace kernel::drivers::block::drivemgr {

enum type_t
{
    NVME,
    SATA,
    SATAPI,
};

enum partStyle
{
    MBR,
    GPT
};

enum partFlags
{
    PRESENT,
    BOOTABLE,
    EFISYS
};

struct [[gnu::packed]] MBRPart
{
    uint8_t Flags;
    uint8_t CHSFirst[3];
    uint8_t Type;
    uint8_t CHSLast[3];
    uint32_t LBAFirst;
    uint32_t Sectors;
};

struct [[gnu::packed]] MBRHdr
{
    uint8_t Bootstrap[440];
    uint32_t UniqueID;
    uint16_t Reserved;
    MBRPart Partitions[4];
    uint8_t Signature[2];
};

struct [[gnu::packed]] GPTPart
{
    uint64_t TypeLow;
    uint64_t TypeHigh;
    uint64_t GUIDLow;
    uint64_t GUIDHigh;
    uint64_t StartLBA;
    uint64_t EndLBA;
    uint64_t Attributes;
    char Label[72];
};

struct [[gnu::packed]] GPTHdr
{
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HdrSize;
    uint32_t HdrCRC32;
    uint32_t Reserved;
    uint64_t LBA;
    uint64_t ALTLBA;
    uint64_t FirstBlock;
    uint64_t LastBlock;
    uint64_t GUIDLow;
    uint64_t GUIDHigh;
    uint64_t PartLBA;
    uint32_t PartCount;
    uint32_t EntrySize;
    uint32_t PartCRC32;
};

struct partTable
{
    MBRHdr mbr;
    GPTHdr gpt;
};

struct Partition;
struct Drive
{
    char name[32] = "New drive";
    uint8_t *buffer = nullptr;
    partTable parttable;
    partStyle partstyle;
    vector<Partition*> partitions;
    type_t type;
    uint64_t uniqueid;

    virtual bool read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
    {
        error("Read function for this device is not available!");
        return false;
    }

    virtual bool write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
    {
        error("Write function for this device is not available!");
        return false;
    }
};

struct Partition
{
    char Label[72];
    uint64_t StartLBA;
    uint64_t EndLBA;
    uint64_t Sectors;
    uint64_t Flags;
    partStyle partstyle;
    Drive *parent;
    size_t i;

    bool read(size_t offset, size_t count, uint8_t *buffer)
    {
        if (count > Sectors - offset) return false;
        return parent->read(StartLBA + offset, count, buffer);
    }
    bool write(size_t offset, size_t count, uint8_t *buffer)
    {
        if (count > Sectors - offset) return false;
        return parent->write(StartLBA + offset, count, buffer);
    }
};

extern bool initialised;
extern vector<Drive*> drives;

void init();
}