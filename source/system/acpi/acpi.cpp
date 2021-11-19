// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/string.hpp>
#include <main.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::acpi {

bool initialised = false;

bool use_xstd;
RSDP *rsdp;

MCFGHeader *mcfg;
FADTHeader *fadt;
SDTHeader *rsdt;

void init()
{
    serial::info("Initialising ACPI");

    if (initialised)
    {
        serial::info("ACPI has already been initialised!\n");
        return;
    }

    rsdp = (RSDP*)rsdp_tag->rsdp;

    if (rsdp->revision >= 2 && rsdp->xsdtaddr)
    {
        use_xstd = true;
        rsdt = (SDTHeader*)rsdp->xsdtaddr;
        serial::info("Found XSDT at: 0x%X", rsdp->xsdtaddr);
    }
    else
    {
        use_xstd = false;
        rsdt = (SDTHeader*)((uintptr_t)rsdp->rsdtaddr);
        serial::info("Found RSDT at: 0x%X", rsdp->rsdtaddr);
    }

    mcfg = (MCFGHeader*)acpi::findtable((char*)"MCFG");
    fadt = (FADTHeader*)acpi::findtable((char*)"FACP");

    serial::newline();
    initialised = true;
}

void *findtable(char *signature)
{
    int entries = (rsdt->length + sizeof(SDTHeader)) / 8;

    for (int t = 0; t < entries; t++)
    {
        SDTHeader *newsdthdr;
        if (use_xstd) newsdthdr = (SDTHeader*)*(uint64_t*)((uint64_t)rsdt + sizeof(SDTHeader) + (t * 8));
        else newsdthdr = (SDTHeader*)((uintptr_t)*(uint32_t*)((uint32_t)((uintptr_t)rsdt) + sizeof(SDTHeader) + (t * 4)));
        
        if (!newsdthdr || !strcmp((const char*)newsdthdr->signature, "")) continue;

        if (!strncmp((const char*)newsdthdr->signature, signature, 4)) return newsdthdr;
        else continue;
    }
    return 0;
}
}