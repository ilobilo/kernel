#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/acpi/acpi.hpp>
#include <system/acpi/pci.hpp>
#include <main.hpp>

bool use_xstd;
RSDP* rsdp;

void ACPI_init()
{
    serial_info("Initializing ACPI");

    rsdp = (RSDP*)rsdp_tag->rsdp;

    sdt_header* rsdt;

    if (rsdp->revision >= 2 && rsdp->xsdtaddr)
    {
        use_xstd = true;
        rsdt = (sdt_header*)rsdp->xsdtaddr;
        serial_info("ACPI: Found XSDT at: 0x%X\n", rsdt);
    }
    else
    {
        use_xstd = false;
        rsdt = (sdt_header*)rsdp->rstdaddr;
        serial_info("Found RSDT at: 0x%X\n", rsdt);
    }

    mcfg_header* mcfg = (mcfg_header*)findtable(rsdt, (char*)"MCFG");

    enumpci(mcfg);

    serial_printf("\n");
    
    serial_info("Initialized ACPI\n");
}

void* findtable(sdt_header* sdthdr, char* signature)
{
    int entries = (sdthdr->length + sizeof(sdt_header)) / 8;
    for (int t = 0; t < entries; t++)
    {
        sdt_header* newsdthdr;
        if (use_xstd)
        {
            newsdthdr = (sdt_header*)*(uint64_t*)((uint64_t)sdthdr + sizeof(sdt_header) + (t * 8));
        }
        else
        {
            newsdthdr = (sdt_header*)*(uint32_t*)(*((uint32_t*)(&sdthdr)) + sizeof(sdt_header) + (t * 4));
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