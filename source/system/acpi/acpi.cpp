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

    mcfg = (MCFGHeader*)acpi::findtable("MCFG");
    fadt = (FADTHeader*)acpi::findtable("FACP");

    serial::newline();
    initialised = true;
}

void *findtable(const char *signature)
{
    size_t entries;
    if (use_xstd) entries = (rsdt->length - sizeof(SDTHeader)) / 8;
    else entries = (rsdt->length - sizeof(SDTHeader)) / 4;
    for (size_t i = 0; i < entries; i++)
    {
        SDTHeader *newsdthdr;
        if (use_xstd) newsdthdr = (SDTHeader*)*(uint64_t*)((uint64_t)rsdt + sizeof(SDTHeader) + (i * 8));
        else newsdthdr = (SDTHeader*)((uintptr_t)*(uint32_t*)((uint32_t)((uintptr_t)rsdt) + sizeof(SDTHeader) + (i * 4)));
        
        if (!newsdthdr || !strcmp((const char*)newsdthdr->signature, "")) continue;

        if (!strncmp((const char*)newsdthdr->signature, signature, 4)) return newsdthdr;
        else continue;
    }
    return 0;
}
}