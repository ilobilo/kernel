// Copyright (C) 2021-2022  ilobilo

#include <drivers/block/drivemgr/drivemgr.hpp>
#include <drivers/block/ahci/ahci.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <drivers/block/ata/ata.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::fs;

namespace kernel::drivers::block::drivemgr {

bool initialised = false;
vector<Drive*> drives;

void addDrive(Drive *drive, type_t type)
{
    serial::newline();
    log("Registering drive #%zu", drives.size());
    drives.push_back(drive);
    drive->type = type;

    std::string prefix("sd");
    prefix.push_back('a' + drives.size() - 1);
    devfs::add(drive, prefix);

    drive->read(nullptr, drive->buffer, 0, 1024);
    memcpy(&drive->parttable, drive->buffer, sizeof(partTable));

    if (drive->parttable.gpt.Signature == GPT_SIGNATURE)
    {
        log("- Found GPT!");
        uint32_t entries_pr = 512 / drive->parttable.gpt.EntrySize;
        uint32_t sectors = drive->parttable.gpt.PartCount / entries_pr;

        for (uint8_t block = 0; block < sectors; block++)
        {
            drive->read(nullptr, drive->buffer, 1024 + block * 512, 2 * 512);
            for (uint8_t part = 0; part < entries_pr; part++)
            {
                GPTPart gptpart = reinterpret_cast<GPTPart*>(drive->buffer)[part];
                if (gptpart.TypeLow || gptpart.TypeHigh)
                {
                    Partition *partition = new Partition;
                    partition->start = gptpart.StartLBA * drive->stat.blksize;
                    partition->sectors = gptpart.EndLBA - gptpart.StartLBA;
                    partition->parent = drive;

                    partition->flags = PRESENT;
                    if (gptpart.Attributes & 1) partition->flags |= EFISYS;

                    partition->stat.blocks = partition->sectors;
                    partition->stat.blksize = drive->stat.blksize;
                    partition->stat.size = partition->sectors * partition->stat.blksize;
                    partition->stat.rdev = vfs::dev_new_id();
                    partition->stat.mode = 0644 | vfs::stats::ifblk;

                    drive->partitions.push_back(partition);

                    std::string name(prefix.c_str());
                    name.push_back(static_cast<char>(drives.size() - 1 + '0'));
                    devfs::add(partition, name);
                }
            }
        }
        log("- Partition count: %zu", drive->partitions.size());
    }
    else if (drive->parttable.mbr.Signature[0] == 0x55 && drive->parttable.mbr.Signature[1] == 0xAA)
    {
        log("- Found MBR!");
        for (size_t p = 0; p < 4; p++)
        {
            if (drive->parttable.mbr.Partitions[p].Type != 0 && drive->parttable.mbr.Partitions[p].Type != 0xEE)
            {
                Partition *partition = new Partition;
                partition->start = drive->parttable.mbr.Partitions[p].LBAFirst;
                partition->sectors = drive->parttable.mbr.Partitions[p].Sectors;
                partition->parent = drive;

                partition->flags = PRESENT;
                if (drive->parttable.mbr.Partitions[p].Type & (1 << 7)) partition->flags |= BOOTABLE;

                partition->stat.blocks = partition->sectors;
                partition->stat.blksize = drive->stat.blksize;
                partition->stat.size = partition->sectors * partition->stat.blksize;
                partition->stat.rdev = vfs::dev_new_id();
                partition->stat.mode = 0644 | vfs::stats::ifblk;

                drive->partitions.push_back(partition);

                std::string name(prefix.c_str());
                name.push_back(static_cast<char>(drives.size() - 1 + '0'));
                devfs::add(partition, name);
            }
        }
        log("- Partition count: %zu", drive->partitions.size());
    }
    else warn("- No partition table found!");
}

void addAHCI()
{
    for (size_t i = 0; i < ahci::devices.size(); i++)
    {
        for (size_t t = 0; t < ahci::devices[i]->ports.size(); t++)
        {
            addDrive(ahci::devices[i]->ports[t], (ahci::devices[i]->ports[t]->portType == ahci::SATA ? SATA : SATAPI));
        }
    }
}

void addATA()
{
    for (size_t i = 0; i < ata::devices.size(); i++)
    {
        for (size_t t = 0; t < ata::devices[i]->ports.size(); t++)
        {
            addDrive(ata::devices[i]->ports[t], (ata::devices[i]->ports[t]->portType == ata::ATA ? ATA : ATAPI));
        }
    }
}

void init()
{
    log("Initialising drive manager");

    if (initialised)
    {
        warn("Drive manager has already been initialised!\n");
        return;
    }

    addAHCI();
    addATA();

    serial::newline();
    initialised = true;
}
}