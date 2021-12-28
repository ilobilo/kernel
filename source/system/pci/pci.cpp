// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <system/pci/pcidesc.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;

namespace kernel::system::pci {

bool initialised = false;
bool legacy = false;

vector<pcidevice_t*> pcidevices;

uint32_t pcidevice_t::get_bar(uint8_t type)
{
    if (legacy)
    {
        uint32_t bar = this->readl(PCI_BAR0);
        if ((bar & 0x01) == type) return bar;
        bar = this->readl(PCI_BAR1);
        if ((bar & 0x01) == type) return bar;
        bar = this->readl(PCI_BAR2);
        if ((bar & 0x01) == type) return bar;
        bar = this->readl(PCI_BAR3);
        if ((bar & 0x01) == type) return bar;
        bar = this->readl(PCI_BAR4);
        if ((bar & 0x01) == type) return bar;
        bar = this->readl(PCI_BAR5);
        if ((bar & 0x01) == type) return bar;
    }
    else
    {
        pciheader0 *hdr = reinterpret_cast<pciheader0*>(this->device);
        uint32_t bar = hdr->BAR0;
        if ((bar & 0x01) == type) return bar;
        bar = hdr->BAR1;
        if ((bar & 0x01) == type) return bar;
        bar = hdr->BAR2;
        if ((bar & 0x01) == type) return bar;
        bar = hdr->BAR3;
        if ((bar & 0x01) == type) return bar;
        bar = hdr->BAR4;
        if ((bar & 0x01) == type) return bar;
        bar = hdr->BAR5;
        if ((bar & 0x01) == type) return bar;
    }
    return 0xFFFFFFFF;
}

pcidevice_t *search(uint8_t Class, uint8_t subclass, uint8_t progif, int skip)
{
    if (!initialised)
    {
        error("PCI has not been initialised!\n");
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

pcidevice_t *search(uint16_t vendor, uint16_t device, int skip)
{
    if (!initialised)
    {
        error("PCI has not been initialised!\n");
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
        error("PCI has not been initialised!\n");
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
        error("PCI has not been initialised!\n");
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

void enumfunc(uint64_t devaddr, uint64_t func, uint64_t dev, uint64_t bus)
{
    uint64_t offset = func << 12;
    uint64_t funcaddr = devaddr + offset;

    pciheader_t *pcidevice = reinterpret_cast<pciheader_t*>(funcaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    pcidevice_t *tpcidevice = new pcidevice_t;
    tpcidevice->device = pcidevice;

    tpcidevice->bus = bus;
    tpcidevice->dev = dev;
    tpcidevice->func = func;

    tpcidevice->vendorstr = getvendorname(tpcidevice->device->vendorid);
    tpcidevice->devicestr = getdevicename(tpcidevice->device->vendorid, tpcidevice->device->deviceid);
    tpcidevice->progifstr = getprogifname(tpcidevice->device->Class, tpcidevice->device->subclass, tpcidevice->device->progif);
    tpcidevice->subclassStr = getsubclassname(tpcidevice->device->Class, tpcidevice->device->subclass);
    tpcidevice->ClassStr = device_classes[tpcidevice->device->Class];

    pcidevices.push_back(tpcidevice);

    log("%.4X:%.4X %s %s",
        pcidevices.back()->device->vendorid,
        pcidevices.back()->device->deviceid,
        pcidevices.back()->vendorstr,
        pcidevices.back()->devicestr);
}

void enumdevice(uint64_t busaddr, uint64_t dev, uint64_t bus)
{
    uint64_t offset = dev << 15;
    uint64_t devaddr = busaddr + offset;

    pciheader_t *pcidevice = reinterpret_cast<pciheader_t*>(devaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    for (uint64_t func = 0; func < 8; func++)
    {
        enumfunc(devaddr, func, dev, bus);
    }
}

void enumbus(uint64_t baseaddr, uint64_t bus)
{
    uint64_t offset = bus << 20;
    uint64_t busaddr = baseaddr + offset;

    pciheader_t *pcidevice = reinterpret_cast<pciheader_t*>(busaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    for (uint64_t dev = 0; dev < 32; dev++)
    {
        enumdevice(busaddr, dev, bus);
    }
}

void checkbus(size_t bus)
{
    for (size_t dev = 0; dev < 32; dev++)
    {
        for (size_t func = 0; func < 8; func++)
        {
            if (readl(bus, dev, func, 0) == 0xFFFFFFFF) continue;

            pcidevice_t *tpcidevice = new pcidevice_t;
            tpcidevice->device = new pciheader_t;

            tpcidevice->bus = bus;
            tpcidevice->dev = dev;
            tpcidevice->func = func;

            uint32_t config_0 = tpcidevice->readl(0);
            uint32_t config_4 = tpcidevice->readl(0x4);
            uint32_t config_8 = tpcidevice->readl(0x8);
            uint32_t config_c = tpcidevice->readl(0xC);

            tpcidevice->device->vendorid = static_cast<uint16_t>(config_0);
            tpcidevice->device->deviceid = static_cast<uint16_t>(config_0 >> 16);
            tpcidevice->device->command = static_cast<uint16_t>(config_4);
            tpcidevice->device->status = static_cast<uint16_t>(config_4 >> 16);
            tpcidevice->device->revisionid = static_cast<uint8_t>(config_8);
            tpcidevice->device->progif = static_cast<uint8_t>(config_8 >> 8);
            tpcidevice->device->subclass = static_cast<uint8_t>(config_8 >> 16);
            tpcidevice->device->Class = static_cast<uint8_t>(config_8 >> 24);
            tpcidevice->device->cachelinesize = static_cast<uint8_t>(config_c);
            tpcidevice->device->latencytimer = static_cast<uint8_t>(config_c >> 8);
            tpcidevice->device->headertype = static_cast<uint8_t>(config_c >> 16);
            tpcidevice->device->bist = static_cast<uint8_t>(config_c >> 24);

            tpcidevice->vendorstr = getvendorname(tpcidevice->device->vendorid);
            tpcidevice->devicestr = getdevicename(tpcidevice->device->vendorid, tpcidevice->device->deviceid);
            tpcidevice->progifstr = getprogifname(tpcidevice->device->Class, tpcidevice->device->subclass, tpcidevice->device->progif);
            tpcidevice->subclassStr = getsubclassname(tpcidevice->device->Class, tpcidevice->device->subclass);
            tpcidevice->ClassStr = device_classes[tpcidevice->device->Class];

            pcidevices.push_back(tpcidevice);

            log("%.4X:%.4X %s %s",
                pcidevices.back()->device->vendorid,
                pcidevices.back()->device->deviceid,
                pcidevices.back()->vendorstr,
                pcidevices.back()->devicestr);

            if (tpcidevice->device->Class == 0x06 && tpcidevice->device->subclass == 0x04)
            {
                checkbus(reinterpret_cast<pciheader1*>(tpcidevice->device)->secBus);
            }
        }
    }
}

void init()
{
    log("Initialising PCI");

    if (initialised)
    {
        warn("PCI has already been initialised!\n");
        return;
    }
    if (!acpi::mcfghdr)
    {
        warn("MCFG was not found!");
        legacy = true;
    }

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
        if ((static_cast<uint8_t>(readl(0, 0, 0, 0xC) >> 16) & 0x80) == 0) checkbus(0);
        else
        {
            for (size_t func = 0; func < 8; func++)
            {
                if (readl(0, 0, func, 0) != 0xFFFF) break;
                checkbus(func);
            }
        }
    }

    serial::newline();
    initialised = true;
}
}