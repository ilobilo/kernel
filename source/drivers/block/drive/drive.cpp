// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/printf.h>
#include <drivers/display/serial/serial.hpp>
#include <drivers/block/drive/drive.hpp>
#include <drivers/block/ahci/ahci.hpp>

using namespace kernel::drivers::display;

namespace kernel::drivers::block::drivemgr {

bool initialised = false;
vector<Drive*> drives;

void addAHCI()
{
    for (size_t i = 0; i < ahci::devices.size(); i++)
    {
        for (size_t t = 0; t < ahci::devices[i]->portCount; t++)
        {
            drives.push_back(ahci::devices[i]->ports[t]);
            sprintf(drives.last()->name, "Disk #%zu", drives.size() - 1);
        }
    }
}

void init()
{
    serial::info("Initialising drive manager");

    if (initialised)
    {
        serial::warn("Drive manager has already been initialised!\n");
        return;
    }

    addAHCI();

    serial::info("Registered drives:");
    for (size_t i = 0; i < drives.size(); i++)
    {
        serial::info("- %s", drives[i]->name);
    }

    serial::newline();
    initialised = true;
}
}