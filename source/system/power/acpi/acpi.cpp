#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/power/acpi/acpi.hpp>
#include <main.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::power::acpi {

bool initialised = false;

bool use_xstd;
RSDP *rsdp;

MCFGHeader *mcfg;
FADTHeader *fadt;

void init()
{
    serial::info("Initialising ACPI");

    if (initialised)
    {
        serial::info("ACPI has already been initialised!\n");
        return;
    }

    rsdp = (RSDP*)rsdp_tag->rsdp;

    SDTHeader *rsdt;

    if (rsdp->revision >= 2 && rsdp->xsdtaddr)
    {
        use_xstd = true;
        rsdt = (SDTHeader*)rsdp->xsdtaddr;
        serial::info("Found XSDT at: 0x%X", rsdt);
    }
    else
    {
        use_xstd = false;
        rsdt = (SDTHeader*)rsdp->rsdtaddr;
        serial::info("Found RSDT at: 0x%X", rsdt);
    }

    mcfg = (MCFGHeader*)acpi::findtable(rsdt, (char*)"MCFG");
    fadt = (FADTHeader*)acpi::findtable(rsdt, (char*)"FACP");

    initialised = true;

    serial::newline();
}

void *findtable(SDTHeader *sdthdr, char *signature)
{
    int entries = (sdthdr->length + sizeof(SDTHeader)) / 8;
    for (int t = 0; t < entries; t++)
    {
        SDTHeader *newsdthdr;
        if (use_xstd)
        {
            newsdthdr = (SDTHeader*)*(uint64_t*)((uint64_t)sdthdr + sizeof(SDTHeader) + (t * 8));
        }
        else
        {
            newsdthdr = (SDTHeader*)*(uint32_t*)(*((uint32_t*)(&sdthdr)) + sizeof(SDTHeader) + (t * 4));
        }
        for (int i = 0; i < 4; i++)
        {
            if (newsdthdr->signature[i] != signature[i])
            {
                break;
            }
            else if (i == 3) return newsdthdr;
        }
    }

    return 0;
}
}