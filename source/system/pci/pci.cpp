// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;

namespace kernel::system::pci {

bool initialised = false;
bool legacy = false;

static uint64_t currbus, currdev, currfunc;

vector<translatedpcidevice_t*> pcidevices;

static void get_addr(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3)) | 0x80000000;
    outl(0xcf8, address);
}

uint8_t readb(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    get_addr(bus, dev, func, offset);
    return inb(0xcfc + (offset & 3));
}

uint16_t readw(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    get_addr(bus, dev, func, offset);
    return inw(0xcfc + (offset & 3));
}

uint32_t readl(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset)
{
    get_addr(bus, dev, func, offset);
    return inl(0xcfc + (offset & 3));
}

void writew(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint16_t value)
{
    get_addr(bus, dev, func, offset);
    outw(0xcfc + (offset & 3), value);
}

void writeb(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint8_t value)
{
    get_addr(bus, dev, func, offset);
    outb(0xcfc + (offset & 3), value);
}

void writel(uint8_t bus, uint8_t dev, uint8_t func, uint32_t offset, uint32_t value)
{
    get_addr(bus, dev, func, offset);
    outl(0xcfc + (offset & 3), value);
}

translatedpcidevice_t *search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip)
{
    if (!initialised)
    {
        serial::err("PCI has not been initialised!\n");
        return nullptr;
    }
    for (uint64_t i = 0; i < pcidevices.size(); i++)
    {
        if (pcidevices[i]->device->Class == Class)
        {
            if (pcidevices[i]->device->subclass == subclass)
            {
                if (pcidevices[i]->device->progif == progif)
                {
                    if (skip > 0)
                    {
                        skip--;
                        continue;
                    }
                    else return pcidevices[i];
                }
            }
        }
    }
    return nullptr;
}

translatedpcidevice_t *search(uint16_t vendor, uint16_t device, int skip)
{
    if (!initialised)
    {
        serial::err("PCI has not been initialised!\n");
        return nullptr;
    }
    for (uint64_t i = 0; i < pcidevices.size(); i++)
    {
        if (pcidevices[i]->device->vendorid == vendor)
        {
            if (pcidevices[i]->device->deviceid == device)
            {
                if (skip > 0)
                {
                    skip--;
                    continue;
                }
                else return pcidevices[i];
            }
        }
    }
    return nullptr;
}

size_t count(uint16_t vendor, uint16_t device)
{
    if (!initialised)
    {
        serial::err("PCI has not been initialised!\n");
        return 0;
    }
    size_t num = 0;
    for (uint64_t i = 0; i < pcidevices.size(); i++)
    {
        if (pcidevices[i]->device->vendorid == vendor)
        {
            if (pcidevices[i]->device->deviceid == device) num++;
        }
    }
    return num;
}

size_t count(uint8_t Class, uint8_t subclass, uint8_t progif)
{
    if (!initialised)
    {
        serial::err("PCI has not been initialised!\n");
        return 0;
    }
    size_t num = 0;
    for (uint64_t i = 0; i < pcidevices.size(); i++)
    {
        if (pcidevices[i]->device->Class == Class)
        {
            if (pcidevices[i]->device->subclass == subclass)
            {
                if (pcidevices[i]->device->progif == progif) num++;
            }
        }
    }
    return num;
}

translatedpcidevice_t *translate(pcidevice_t* device)
{
    translatedpcidevice_t *pcidevice = new translatedpcidevice_t;

    pcidevice->device = device;

    pcidevice->vendorstr = getvendorname(device->vendorid);
    pcidevice->devicestr = getdevicename(device->vendorid, device->deviceid);
    pcidevice->progifstr = getprogifname(device->Class, device->subclass, device->progif);
    pcidevice->subclassStr = getsubclassname(device->Class, device->subclass);
    pcidevice->ClassStr = device_classes[device->Class];

    pcidevice->bus = currbus;
    pcidevice->dev = currdev;
    pcidevice->func = currfunc;

    return pcidevice;
}

void enumfunc(uint64_t devaddr, uint64_t func)
{
    uint64_t offset = func << 12;
    uint64_t funcaddr = devaddr + offset;

    pcidevice_t *pcidevice = reinterpret_cast<pcidevice_t*>(funcaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    currfunc = func;

    pcidevices.push_back(translate(pcidevice));
    serial::info("%.4X:%.4X %s %s",
        pcidevices.last()->device->vendorid,
        pcidevices.last()->device->deviceid,
        pcidevices.last()->vendorstr,
        pcidevices.last()->devicestr);
}

void enumdevice(uint64_t busaddr, uint64_t dev)
{
    uint64_t offset = dev << 15;
    uint64_t devaddr = busaddr + offset;

    pcidevice_t *pcidevice = reinterpret_cast<pcidevice_t*>(devaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    currdev = dev;

    for (uint64_t func = 0; func < 8; func++)
    {
        enumfunc(devaddr, func);
    }
}

void enumbus(uint64_t baseaddr, uint64_t bus)
{
    uint64_t offset = bus << 20;
    uint64_t busaddr = baseaddr + offset;

    pcidevice_t *pcidevice = reinterpret_cast<pcidevice_t*>(busaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    currbus = bus;

    for (uint64_t dev = 0; dev < 32; dev++)
    {
        enumdevice(busaddr, dev);
    }
}

void init()
{
    serial::info("Initialising PCI");

    if (initialised)
    {
        serial::warn("PCI has already been initialised!\n");
        return;
    }
    if (!acpi::mcfghdr)
    {
        serial::err("MCFG was not found");
        serial::warn("This might take some time");
        legacy = true;
    }

    pcidevices.init(5);
    if (!legacy)
    {
        int entries = ((acpi::mcfghdr->header.length) - sizeof(acpi::MCFGHeader)) / sizeof(acpi::deviceconfig);
        for (int t = 0; t < entries; t++)
        {
            acpi::deviceconfig *newdevconf = reinterpret_cast<acpi::deviceconfig*>(reinterpret_cast<uint64_t>(acpi::mcfghdr) + sizeof(acpi::MCFGHeader) + (sizeof(acpi::deviceconfig) * t));
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
                    uint32_t config_0 = readl(bus, dev, func, 0);
                    if (config_0 == 0xFFFFFFFF) continue;

                    uint32_t config_4 = readl(bus, dev, func, 0x4);
                    uint32_t config_8 = readl(bus, dev, func, 0x8);
                    uint32_t config_c = readl(bus, dev, func, 0xc);

                    pcidevice_t *pcidevice = new pcidevice_t;

                    pcidevice->vendorid = static_cast<uint16_t>(config_0);
                    pcidevice->deviceid = static_cast<uint16_t>(config_0 >> 16);
                    pcidevice->command = static_cast<uint16_t>(config_4);
                    pcidevice->status = static_cast<uint16_t>(config_4 >> 16);
                    pcidevice->revisionid = static_cast<uint8_t>(config_8);
                    pcidevice->progif = static_cast<uint8_t>(config_8 >> 8);
                    pcidevice->subclass = static_cast<uint8_t>(config_8 >> 16);
                    pcidevice->Class = static_cast<uint8_t>(config_8 >> 24);
                    pcidevice->cachelinesize = static_cast<uint8_t>(config_c);
                    pcidevice->latencytimer = static_cast<uint8_t>(config_c >> 8);
                    pcidevice->headertype = static_cast<uint8_t>(config_c >> 16);
                    pcidevice->bist = static_cast<uint8_t>(config_c >> 24);

                    currbus = bus;
                    currdev = dev;
                    currfunc = func;

                    pcidevices.push_back(translate(pcidevice));

                    serial::info("%.4X:%.4X %s %s",
                        pcidevices.last()->device->vendorid,
                        pcidevices.last()->device->deviceid,
                        pcidevices.last()->vendorstr,
                        pcidevices.last()->devicestr);
                }
            }
        }
    }

    serial::newline();
    initialised = true;
}
}