#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/power/acpi/acpi.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/io.hpp>

bool pci_initialised = false;
bool pci_legacy = false;
bool use_pciids = false;

translatedpcideviceheader **pcidevices;
uint64_t pciAllocate = 10;
uint64_t pcidevcount = 0;

translatedpcideviceheader *PCI_search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip)
{
    if (!pci_initialised)
    {
        serial_info("PCI has not been initialised!\n");
        return NULL;
    }
    for (uint64_t i = 0; i < pcidevcount; i++)
    {
        if (pcidevices[i]->Class == Class)
        {
            if (pcidevices[i]->subclass == subclass)
            {
                if (pcidevices[i]->progif == progif)
                {
                    if (skip > 0)
                    {
                        skip--;
                        continue;
                    }
                    else return pcidevices[i];
                }
                else continue;
            }
            else continue;
        }
        else continue;
    }
    return NULL;
}

translatedpcideviceheader *PCI_translate(pcideviceheader* device)
{
    translatedpcideviceheader *pcidevice = (translatedpcideviceheader*)malloc(sizeof(translatedpcideviceheader));

    pcidevice->vendorid = device->vendorid;
    pcidevice->deviceid = device->deviceid;
    if (use_pciids)
    {
        char *buffer = (char*)malloc(100 * sizeof(char));
        pcidevice->vendorstr = strdup(getvendorname(device->vendorid, buffer));
        if (!strcmp(pcidevice->vendorstr, "")) pcidevice->vendorstr = strdup(getvendorname(device->vendorid));

        pcidevice->devicestr = strdup(getdevicename(device->vendorid, device->deviceid, buffer));
        if (!strcmp(pcidevice->devicestr, "")) pcidevice->devicestr = strdup(getvendorname(device->deviceid));
        free(buffer);
    }
    else
    {
        pcidevice->vendorstr = getvendorname(device->vendorid);
        pcidevice->devicestr = getdevicename(device->vendorid, device->deviceid);
    }
    pcidevice->command = device->command;
    pcidevice->status = device->status;
    pcidevice->revisionid = device->revisionid;
    pcidevice->progif = device->progif;
    pcidevice->progifstr = getprogifname(device->Class, device->subclass, device->progif);
    pcidevice->subclass = device->subclass;
    pcidevice->subclassStr = getsubclassname(device->Class, device->subclass);
    pcidevice->Class = device->Class;
    pcidevice->ClassStr = device_classes[device->Class];
    pcidevice->cachelinesize = device->cachelinesize;
    pcidevice->latencytimer = device->latencytimer;
    pcidevice->headertype = device->headertype;
    pcidevice->bist = device->bist;

    return pcidevice;
}

void PCI_add(pcideviceheader *device)
{
    if (pcidevcount >= (alloc_getsize(pcidevices) / sizeof(translatedpcideviceheader)))
    {
        pciAllocate += 10;
        pcidevices = (translatedpcideviceheader**)realloc(pcidevices, pciAllocate * sizeof(translatedpcideviceheader));
    }
    if (pcidevcount < (alloc_getsize(pcidevices) / sizeof(translatedpcideviceheader)))
    {
        pcidevices[pcidevcount] = PCI_translate(device);
        pcidevcount++;
    }
    else
    {
        serial_newline();
        serial_err("Could not add pci device to the list!");
        serial_err("Possible reason: Could not allocate memory\n");
    }
}

uint16_t PCI_read(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset)
{
    uint64_t address;
    uint64_t lbus = (uint64_t)bus;
    uint64_t lslot = (uint64_t)slot;
    uint64_t lfunc = (uint64_t)func;
    uint16_t tmp = 0;
    address = (uint64_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    outl(0xCF8, address);
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

uint16_t PCI_getvenid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = PCI_read(bus, dev, func, 0);
    return r0;
}
uint16_t PCI_getdevid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = PCI_read(bus, dev, func, 2);
    return r0;
}
uint16_t PCI_getclassid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = PCI_read(bus, dev, func, 0xA);
    return (r0 & ~0x00FF) >> 8;
}
uint16_t PCI_getsubclassid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = PCI_read(bus, dev, func, 0xA);
    return (r0 & ~0xFF00);
}

void enumfunc(uint64_t deviceaddr, uint64_t func)
{
    uint64_t offset = func << 12;
    uint64_t funcaddr = deviceaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)funcaddr;
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    PCI_add(pcidevice);
    serial_info("%s / %s / %s / %s / %s",
        pcidevices[pcidevcount - 1]->vendorstr,
        pcidevices[pcidevcount - 1]->devicestr,
        pcidevices[pcidevcount - 1]->ClassStr,
        pcidevices[pcidevcount - 1]->subclassStr,
        pcidevices[pcidevcount - 1]->progifstr);
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
        serial_err("MCFG was not found");
        serial_info("Using legacy way\n");
        pci_legacy = true;
    }
    if (!acpi_initialised)
    {
        serial_info("ACPI has not been initialised!\n");
        ACPI_init();
    }

    bool temp = alloc_debug;
    alloc_debug = false;

    if (use_pciids) ustar_search("/pci.ids", &PCIids);

    pcidevices = (translatedpcideviceheader**)malloc(pciAllocate * sizeof(translatedpcideviceheader));

    if (!pci_legacy)
    {
        int entries = ((mcfg->header.length) - sizeof(MCFGHeader)) / sizeof(deviceconfig);
        for (int t = 0; t < entries; t++)
        {
            deviceconfig *newdevconf = (deviceconfig*)((uint64_t)mcfg + sizeof(MCFGHeader) + (sizeof(deviceconfig) * t));
            for (uint64_t bus = newdevconf->startbus; bus < newdevconf->endbus; bus++)
            {
                enumbus(newdevconf->baseaddr, bus);
            }
        }
    }
    else
    {
        for (int bus = 0; bus < 256; bus++)
        {
            for (int dev = 0; dev < 32; dev++)
            {
                for (int func = 0; func < 8; func++)
                {
                    pcideviceheader *device = (pcideviceheader*)malloc(sizeof(pcideviceheader));
                    uint16_t vendorid = PCI_getvenid(bus, dev, func);
                    if (vendorid == 0 || vendorid == 0xFFFF) continue;

                    device->vendorid = vendorid;
                    device->deviceid = PCI_getdevid(bus, dev, func);
                    device->Class = PCI_getclassid(bus, dev, func);
                    device->subclass = PCI_getsubclassid(bus, dev, func);

                    PCI_add(device);
                    serial_info("%s / %s / %s / %s / %s",
                        pcidevices[pcidevcount - 1]->vendorstr,
                        pcidevices[pcidevcount - 1]->devicestr,
                        pcidevices[pcidevcount - 1]->ClassStr,
                        pcidevices[pcidevcount - 1]->subclassStr,
                        pcidevices[pcidevcount - 1]->progifstr);
                    free(device);
                }
            }
        }
    }
    serial_newline();

    alloc_debug = temp;
    pci_initialised = true;
}