#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/power/acpi/acpi.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::system::power;
using namespace kernel::system::mm;
using namespace kernel::lib;

namespace kernel::system::pci {

bool initialised = false;
bool legacy = false;
bool use_pciids = false;

translatedpcideviceheader **pcidevices;
uint64_t pciAllocate = 10;
uint64_t pcidevcount = 0;

translatedpcideviceheader *PCI_search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip)
{
    if (!initialised)
    {
        serial::info("PCI has not been initialised!\n");
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

translatedpcideviceheader *translate(pcideviceheader* device)
{
    translatedpcideviceheader *pcidevice = (translatedpcideviceheader*)heap::malloc(sizeof(translatedpcideviceheader));

    pcidevice->vendorid = device->vendorid;
    pcidevice->deviceid = device->deviceid;
    if (use_pciids)
    {
        char *buffer = (char*)heap::malloc(100 * sizeof(char));
        pcidevice->vendorstr = string::strdup(getvendorname(device->vendorid, buffer));
        if (!string::strcmp(pcidevice->vendorstr, "")) pcidevice->vendorstr = string::strdup(getvendorname(device->vendorid));

        pcidevice->devicestr = string::strdup(getdevicename(device->vendorid, device->deviceid, buffer));
        if (!string::strcmp(pcidevice->devicestr, "")) pcidevice->devicestr = string::strdup(getvendorname(device->deviceid));
        heap::free(buffer);
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

void add(pcideviceheader *device)
{
    if (pcidevcount >= (heap::getsize(pcidevices) / sizeof(translatedpcideviceheader)))
    {
        pciAllocate += 10;
        pcidevices = (translatedpcideviceheader**)heap::realloc(pcidevices, pciAllocate * sizeof(translatedpcideviceheader));
    }
    if (pcidevcount < (heap::getsize(pcidevices) / sizeof(translatedpcideviceheader)))
    {
        pcidevices[pcidevcount] = translate(device);
        pcidevcount++;
    }
    else
    {
        serial::newline();
        serial::err("Could not add pci device to the list!");
        serial::err("Possible reason: Could not allocate memory\n");
    }
}

uint16_t read(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset)
{
    uint64_t address;
    uint64_t lbus = (uint64_t)bus;
    uint64_t lslot = (uint64_t)slot;
    uint64_t lfunc = (uint64_t)func;
    uint16_t tmp = 0;
    address = (uint64_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    io::outl(0xCF8, address);
    tmp = (uint16_t)((io::inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

uint16_t getvenid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = read(bus, dev, func, 0);
    return r0;
}
uint16_t getdevid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = read(bus, dev, func, 2);
    return r0;
}
uint16_t getclassid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = read(bus, dev, func, 0xA);
    return (r0 & ~0x00FF) >> 8;
}
uint16_t getsubclassid(uint16_t bus, uint16_t dev, uint16_t func)
{
    uint32_t r0 = read(bus, dev, func, 0xA);
    return (r0 & ~0xFF00);
}

void enumfunc(uint64_t deviceaddr, uint64_t func)
{
    uint64_t offset = func << 12;
    uint64_t funcaddr = deviceaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)funcaddr;
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    add(pcidevice);
    serial::info("%s / %s / %s / %s / %s",
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

void init()
{
    serial::info("Initialising PCI\n");

    if (initialised)
    {
        serial::info("PCI has already been initialised!\n");
        return;
    }
    if (acpi::mcfg == NULL)
    {
        serial::err("MCFG was not found");
        serial::info("Using legacy way\n");
        legacy = true;
    }
    if (!acpi::initialised)
    {
        serial::info("ACPI has not been initialised!\n");
        acpi::init();
    }

    bool temp = heap::debug;
    heap::debug = false;

    if (use_pciids) ustar::search("/pci.ids", &PCIids);

    pcidevices = (translatedpcideviceheader**)heap::malloc(pciAllocate * sizeof(translatedpcideviceheader));

    if (!legacy)
    {
        int entries = ((acpi::mcfg->header.length) - sizeof(acpi::MCFGHeader)) / sizeof(acpi::deviceconfig);
        for (int t = 0; t < entries; t++)
        {
            acpi::deviceconfig *newdevconf = (acpi::deviceconfig*)((uint64_t)acpi::mcfg + sizeof(acpi::MCFGHeader) + (sizeof(acpi::deviceconfig) * t));
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
                    pcideviceheader *device = (pcideviceheader*)heap::malloc(sizeof(pcideviceheader));
                    uint16_t vendorid = getvenid(bus, dev, func);
                    if (vendorid == 0 || vendorid == 0xFFFF) continue;

                    device->vendorid = vendorid;
                    device->deviceid = getdevid(bus, dev, func);
                    device->Class = getclassid(bus, dev, func);
                    device->subclass = getsubclassid(bus, dev, func);

                    add(device);
                    serial::info("%s / %s / %s / %s / %s",
                        pcidevices[pcidevcount - 1]->vendorstr,
                        pcidevices[pcidevcount - 1]->devicestr,
                        pcidevices[pcidevcount - 1]->ClassStr,
                        pcidevices[pcidevcount - 1]->subclassStr,
                        pcidevices[pcidevcount - 1]->progifstr);
                    heap::free(device);
                }
            }
        }
    }
    serial::newline();

    heap::debug = temp;
    initialised = true;
}
}