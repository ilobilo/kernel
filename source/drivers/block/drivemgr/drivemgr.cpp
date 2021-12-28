// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/block/drivemgr/drivemgr.hpp>
#include <drivers/block/ahci/ahci.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::block::drivemgr {

bool initialised = false;
vector<Drive*> drives;

void addDrive(Drive *drive, type_t type)
{
    drives.push_back(drive);
    drive->type = type;
    sprintf(drive->name, "Drive #%zu", drives.size() - 1);

    drive->read(0, 2, drive->buffer);
    memcpy(&drive->parttable, drive->buffer, sizeof(partTable));

    if (drive->parttable.gpt.Signature == 0x5452415020494645)
    {
        log("Found GPT on disk #%zu!", drives.size() - 1);
        drive->partstyle = GPT;

        uint32_t entries_pr = 512 / drive->parttable.gpt.EntrySize;
        uint32_t sectors = drive->parttable.gpt.PartCount / entries_pr;

        for (uint8_t block = 0; block < sectors; block++)
        {
            drive->read(2 + block, 1, drive->buffer);
            for (uint8_t part = 0; part < entries_pr; part++)
            {
                GPTPart gptpart = reinterpret_cast<GPTPart*>(drive->buffer)[part];
                if (gptpart.TypeLow || gptpart.TypeHigh)
                {
                    log("- Found partition #%zu", drive->partitions.size());
                    Partition *partition = new Partition;
                    sprintf(partition->Label, "GPT Part #%zu", drive->partitions.size());
                    partition->StartLBA = gptpart.StartLBA;
                    partition->EndLBA = gptpart.EndLBA;
                    partition->Sectors = partition->EndLBA - partition->StartLBA;
                    partition->Flags = PRESENT;
                    if (gptpart.Attributes & 1) partition->Flags |= EFISYS;
                    partition->partstyle = GPT;
                    partition->parent = drive;
                    drive->partitions.push_back(partition);
                }
            }
        }
    }
    else if (drive->parttable.mbr.Signature[0] == 0x55 && drive->parttable.mbr.Signature[1] == 0xAA)
    {
        log("Found MBR on disk #%zu!", drives.size() - 1);
        drive->partstyle = MBR;

        for (size_t p = 0; p < 4; p++)
        {
            if (drive->parttable.mbr.Partitions[p].LBAFirst != 0)
            {
                log("- Found partition #%zu", drive->partitions.size());
                Partition *partition = new Partition;
                sprintf(partition->Label, "MBR Part #%zu", drive->partitions.size());
                partition->StartLBA = drive->parttable.mbr.Partitions[p].LBAFirst;
                partition->EndLBA = drive->parttable.mbr.Partitions[p].LBAFirst + drive->parttable.mbr.Partitions[p].Sectors;
                partition->Sectors = drive->parttable.mbr.Partitions[p].Sectors;
                partition->Flags = PRESENT | BOOTABLE;
                partition->partstyle = MBR;
                partition->parent = drive;
                drive->partitions.push_back(partition);
            }
        }
    }
    else warn("No partition table present on drive #%zu", drives.size() - 1);
}

void addAHCI()
{
    for (size_t i = 0; i < ahci::devices.size(); i++)
    {
        for (size_t t = 0; t < ahci::devices[i]->portCount; t++)
        {
            addDrive(ahci::devices[i]->ports[t], (ahci::devices[i]->ports[t]->portType == ahci::SATA ? SATA : SATAPI));
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

    serial::newline();
    initialised = true;
}
}