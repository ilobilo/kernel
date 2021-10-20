#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/power/acpi/acpi.hpp>
#include <main.hpp>

bool acpi_initialised = false;

bool use_xstd;
RSDP *rsdp;

MCFGHeader *mcfg;
FADTHeader *fadt;

void ACPI_init()
{
    serial_info("Initialising ACPI");

    if (acpi_initialised)
    {
        serial_info("ACPI has already been initialised!\n");
        return;
    }

    rsdp = (RSDP*)rsdp_tag->rsdp;

    SDTHeader *rsdt;

    if (rsdp->revision >= 2 && rsdp->xsdtaddr)
    {
        use_xstd = true;
        rsdt = (SDTHeader*)rsdp->xsdtaddr;
        serial_info("Found XSDT at: 0x%X", rsdt);
    }
    else
    {
        use_xstd = false;
        rsdt = (SDTHeader*)rsdp->rsdtaddr;
        serial_info("Found RSDT at: 0x%X", rsdt);
    }

    mcfg = (MCFGHeader*)findtable(rsdt, (char*)"MCFG");
    fadt = (FADTHeader*)findtable(rsdt, (char*)"FACP");

    acpi_initialised = true;

    serial_newline();
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