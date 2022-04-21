// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/vfs/vfs.hpp>
#include <lib/string.hpp>
#include <lib/vector.hpp>
#include <lib/log.hpp>
#include <cstdint>

using namespace kernel::system;

namespace kernel::drivers::block::drivemgr {

static constexpr uint64_t GPT_SIGNATURE = 0x5452415020494645;
static constexpr uint64_t SECTOR_SIZE = 512;

enum type_t
{
    NVME,
    SATA,
    SATAPI,
    ATA,
    ATAPI
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
struct Drive : vfs::resource_t
{
    uint8_t *buffer = nullptr;

    partTable parttable;
    vector<Partition*> partitions;
    uint64_t sectors;
    type_t type;
};

struct Partition : vfs::resource_t
{
    uint64_t start;
    uint64_t sectors;
    uint64_t flags;
    vfs::resource_t *parent;

    int64_t read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
    {
        if (offset + size > this->sectors * this->parent->stat.blksize)
        {
            size = this->sectors * this->parent->stat.blksize - offset;
        }
        return this->parent->read(handle, buffer, this->start + offset, size);
    }

    int64_t write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
    {
        if (offset + size > this->sectors * this->parent->stat.blksize)
        {
            size = this->sectors * this->parent->stat.blksize - offset;
        }
        return this->parent->write(handle, buffer, this->start + offset, size);
    }

    int ioctl(void *handle, uint64_t request, void *argp)
    {
        return this->parent->ioctl(handle, request, argp);
    }
    bool grow(void *handle, size_t new_size)
    {
        return this->parent->grow(handle, new_size);
    }
    void unref(void *handle)
    {
        this->parent->unref(handle);
    }
    void link(void *handle)
    {
    }
    void unlink(void *handle)
    {
    }
    void *mmap(uint64_t page, int flags)
    {
        return this->parent->mmap(page, flags);
    }
};

extern bool initialised;
extern vector<Drive*> drives;

void init();
}