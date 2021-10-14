#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/power/acpi/acpi.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>

bool pci_initialised = false;

translatedpcideviceheader *pcidevices;
uint64_t pciAllocate = 10;
uint64_t pcidevcount = 0;

translatedpcideviceheader PCI_search(uint8_t Class, uint8_t subclass, uint8_t progif)
{
    translatedpcideviceheader null;
    if (!pci_initialised)
    {
        serial_info("PCI has not been initialised!\n");
        return null;
    }
    for (uint64_t i = 0; i < pcidevcount; i++)
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

translatedpcideviceheader PCI_translate(pcideviceheader* device)
{
    translatedpcideviceheader pcidevice;

    pcidevice.vendorid = device->vendorid;
    pcidevice.vendorstr = getvendorname(device->vendorid);
    pcidevice.deviceid = device->deviceid;
    pcidevice.devicestr = getdevicename(device->vendorid, device->deviceid);
    pcidevice.command = device->command;
    pcidevice.status = device->status;
    pcidevice.revisionid = device->revisionid;
    pcidevice.progif = device->progif;
    pcidevice.progifstr = getprogifname(device->Class, device->subclass, device->progif);
    pcidevice.subclass = device->subclass;
    pcidevice.subclassStr = getsubclassname(device->Class, device->subclass);
    pcidevice.Class = device->Class;
    pcidevice.ClassStr = device_classes[device->Class];
    pcidevice.cachelinesize = device->cachelinesize;
    pcidevice.latencytimer = device->latencytimer;
    pcidevice.headertype = device->headertype;
    pcidevice.bist = device->bist;

    return pcidevice;
}

void enumfunc(uint64_t deviceaddr, uint64_t func)
{
    uint64_t offset = func << 12;

    uint64_t funcaddr = deviceaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)funcaddr;

    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    if (pcidevcount >= (alloc_getsize(pcidevices) / sizeof(translatedpcideviceheader)))
    {
        pciAllocate += 10;
        pcidevices = (translatedpcideviceheader*)realloc(pcidevices, pciAllocate * sizeof(translatedpcideviceheader));
    }
    if (pcidevcount < (alloc_getsize(pcidevices) / sizeof(translatedpcideviceheader)))
    {
        pcidevices[pcidevcount] = PCI_translate(pcidevice);
    }
    else
    {
        serial_newline();
        serial_err("Could not add pci device to the list!");
        serial_err("Possible reason: Could not allocate memory\n");
    }

    serial_info("%s / %s / %s / %s / %s",
        pcidevices[pcidevcount].vendorstr,
        pcidevices[pcidevcount].devicestr,
        pcidevices[pcidevcount].ClassStr,
        pcidevices[pcidevcount].subclassStr,
        pcidevices[pcidevcount].progifstr);

    pcidevcount++;
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

    bool temp = alloc_debug;
    alloc_debug = false;

    pcidevices = (translatedpcideviceheader*)malloc(pciAllocate * sizeof(translatedpcideviceheader));

    int entries = ((mcfg->header.length) - sizeof(MCFGHeader)) / sizeof(deviceconfig);
    for (int t = 0; t < entries; t++)
    {
        deviceconfig *newdevconf = (deviceconfig*)((uint64_t)mcfg + sizeof(MCFGHeader) + (sizeof(deviceconfig) * t));
        for (uint64_t bus = newdevconf->startbus; bus < newdevconf->endbus; bus++)
        {
            enumbus(newdevconf->baseaddr, bus);
        }
    }

    alloc_debug = temp;

    pci_initialised = true;
}