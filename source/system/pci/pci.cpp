#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/power/acpi/acpi.hpp>
#include <system/heap/heap.hpp>
#include <system/pci/pci.hpp>

bool pci_initialised = false;

pcideviceheader *pcidevices;
uint64_t pciAllocate = 10;
uint64_t pcidevcount = 0;

pcideviceheader PCI_search(uint8_t Class, uint8_t subclass, uint8_t progif)
{
    pcideviceheader null;
    if (!pci_initialised)
    {
        serial_info("PCI has not been initialised!\n");
        return null;
    }
    for (int i = 0; i < pcidevcount; i++)
    {
        if (pcidevices[i].Class == Class)
        {
            if (pcidevices[i].subclass == subclass)
            {
                if (pcidevices[i].progif == progif)
                {
                    return pcidevices[i];
                }
                else continue;
            }
            else continue;
        }
        else continue;
    }
    return null;
}

pcideviceheader PCI_translate(pcideviceheader* device)
{
    pcideviceheader pcidevice = 
    {
        .vendorid = device->vendorid,
        .deviceid = device->deviceid,
        .command = device->command,
        .status = device->status,
        .revisionid = device->revisionid,
        .progif = device->progif,
        .subclass = device->subclass,
        .Class = device->Class,
        .cachelinesize = device->cachelinesize,
        .latencytimer = device->latencytimer,
        .headertype = device->headertype,
        .bist = device->bist
    };
    return pcidevice;
}

void enumfunc(uint64_t deviceaddr, uint64_t func)
{
    uint64_t offset = func << 12;

    uint64_t funcaddr = deviceaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)funcaddr;

    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    if (pcidevcount >= (alloc_getsize(pcidevices) / sizeof(pcideviceheader)))
    {
        pciAllocate += 10;
        pcidevices = (pcideviceheader*)realloc(pcidevices, pciAllocate * sizeof(pcideviceheader));
    }

    serial_info("%s / %s / %s / %s / %s", getvendorname(pcidevice->vendorid),
        getdevicename(pcidevice->vendorid, pcidevice->deviceid),
        device_classes[pcidevice->Class],
        getsubclassname(pcidevice->Class, pcidevice->subclass),
        getprogifname(pcidevice->Class, pcidevice->subclass, pcidevice->progif));

    if (pcidevcount < (alloc_getsize(pcidevices) / sizeof(pcideviceheader)))
    {
        pcidevices[pcidevcount] = PCI_translate(pcidevice);
        pcidevcount++;
    }
    else
    {
        serial_newline();
        serial_err("Could not add pci device to the list!");
        serial_err("Possible reason: Could not allocate memory\n");
    }
}

void enumdevice(uint64_t busaddr, uint64_t device)
{
    uint64_t offset = device << 15;

    uint64_t deviceaddr = busaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)deviceaddr;

    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    for (uint64_t func = 0; func < 8; func++)
    {
        enumfunc(deviceaddr, func);
    }
}

void enumbus(uint64_t baseaddr, uint64_t bus)
{
    uint64_t offset = bus << 20;

    uint64_t busaddr = baseaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)busaddr;

    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    for (uint64_t device = 0; device < 32; device++)
    {
        enumdevice(busaddr, device);
    }
}

void PCI_init()
{
    serial_info("Initialising PCI\n");

    if (pci_initialised)
    {
        serial_info("PCI has already been initialised!\n");
        return;
    }

    if (mcfg == NULL)
    {
        serial_err("Could not initialise PCI devices!");
        serial_err("Possible reasons: MCFG was not found\n");
        return;
    }

    if (!acpi_initialised)
    {
        serial_info("ACPI has not been initialised!\n");
        ACPI_init();
    }

    pcidevices = (pcideviceheader*)malloc(pciAllocate * sizeof(pcideviceheader));

    int entries = ((mcfg->header.length) - sizeof(MCFGHeader)) / sizeof(deviceconfig);
    for (int t = 0; t < entries; t++)
    {
        deviceconfig *newdevconf = (deviceconfig*)((uint64_t)mcfg + sizeof(MCFGHeader) + (sizeof(deviceconfig) * t));
        for (uint64_t bus = newdevconf->startbus; bus < newdevconf->endbus; bus++)
        {
            enumbus(newdevconf->baseaddr, bus);
        }
    }

    pci_initialised = true;
}