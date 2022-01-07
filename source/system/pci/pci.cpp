// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <system/pci/pcidesc.hpp>
#include <system/pci/pci.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;

namespace kernel::system::pci {

bool initialised = false;
bool legacy = false;

vector<pcidevice_t*> devices;

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
        for (const uint32_t bar : hdr->BAR)
        {
            if ((bar & 0x01) == type) return bar;
        }
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
    for (uint64_t i = 0; i < devices.size(); i++)
    {
        if (devices[i]->device->Class == Class)
        {
            if (devices[i]->device->subclass == subclass)
            {
                if (devices[i]->device->progif == progif)
                {
                    if (skip > 0)
                    {
                        skip--;
                        continue;
                    }
                    else return devices[i];
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
    for (uint64_t i = 0; i < devices.size(); i++)
    {
        if (devices[i]->device->vendorid == vendor)
        {
            if (devices[i]->device->deviceid == device)
            {
                if (skip > 0)
                {
                    skip--;
                    continue;
                }
                else return devices[i];
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
    for (uint64_t i = 0; i < devices.size(); i++)
    {
        if (devices[i]->device->vendorid == vendor)
        {
            if (devices[i]->device->deviceid == device) num++;
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
    for (uint64_t i = 0; i < devices.size(); i++)
    {
        if (devices[i]->device->Class == Class)
        {
            if (devices[i]->device->subclass == subclass)
            {
                if (devices[i]->device->progif == progif) num++;
            }
        }
    }
    return num;
}

static void enumfunc(uint64_t devaddr, uint8_t func, uint8_t dev, uint8_t bus, uint16_t seg)
{
    uint64_t offset = func << 12;
    uint64_t funcaddr = devaddr + offset;

    pciheader_t *pcidevice = reinterpret_cast<pciheader_t*>(funcaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    pcidevice_t *tpcidevice = new pcidevice_t;
    tpcidevice->device = pcidevice;

    tpcidevice->seg = seg;
    tpcidevice->bus = bus;
    tpcidevice->dev = dev;
    tpcidevice->func = func;

    tpcidevice->vendorstr = getvendorname(tpcidevice->device->vendorid);
    tpcidevice->devicestr = getdevicename(tpcidevice->device->vendorid, tpcidevice->device->deviceid);
    tpcidevice->progifstr = getprogifname(tpcidevice->device->Class, tpcidevice->device->subclass, tpcidevice->device->progif);
    tpcidevice->subclassStr = getsubclassname(tpcidevice->device->Class, tpcidevice->device->subclass);
    tpcidevice->ClassStr = device_classes[tpcidevice->device->Class];

    devices.push_back(tpcidevice);

    log("%.4X:%.4X %s %s",
        devices.back()->device->vendorid,
        devices.back()->device->deviceid,
        devices.back()->vendorstr,
        devices.back()->devicestr);
}

static void enumdevice(uint64_t busaddr, uint8_t dev, uint8_t bus, uint16_t seg)
{
    uint64_t offset = dev << 15;
    uint64_t devaddr = busaddr + offset;

    pciheader_t *pcidevice = reinterpret_cast<pciheader_t*>(devaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    for (uint8_t func = 0; func < 8; func++)
    {
        enumfunc(devaddr, func, dev, bus, seg);
    }
}

static void enumbus(uint64_t baseaddr, uint8_t bus, uint16_t seg)
{
    uint64_t offset = bus << 20;
    uint64_t busaddr = baseaddr + offset;

    pciheader_t *pcidevice = reinterpret_cast<pciheader_t*>(busaddr);
    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    for (uint8_t dev = 0; dev < 32; dev++)
    {
        enumdevice(busaddr, dev, bus, seg);
    }
}

static void enumbus(uint8_t bus)
{
    for (uint8_t dev = 0; dev < 32; dev++)
    {
        for (uint8_t func = 0; func < 8; func++)
        {
            if (readl(0, bus, dev, func, 0) == 0xFFFFFFFF) continue;

            pcidevice_t *tpcidevice = new pcidevice_t;
            tpcidevice->device = new pciheader_t;

            tpcidevice->seg = 0;
            tpcidevice->bus = bus;
            tpcidevice->dev = dev;
            tpcidevice->func = func;

            uint32_t config_vendor = tpcidevice->readl(PCI_VENDOR_ID);
            uint32_t config_command = tpcidevice->readl(PCI_COMMAND);
            uint32_t config_revision = tpcidevice->readl(PCI_REVISION_ID);
            uint32_t config_cacheline = tpcidevice->readl(PCI_CACHE_LINE_SIZE);

            tpcidevice->device->vendorid = static_cast<uint16_t>(config_vendor);
            tpcidevice->device->deviceid = static_cast<uint16_t>(config_vendor >> 16);
            tpcidevice->device->command = static_cast<uint16_t>(config_command);
            tpcidevice->device->status = static_cast<uint16_t>(config_command >> 16);
            tpcidevice->device->revisionid = static_cast<uint8_t>(config_revision);
            tpcidevice->device->progif = static_cast<uint8_t>(config_revision >> 8);
            tpcidevice->device->subclass = static_cast<uint8_t>(config_revision >> 16);
            tpcidevice->device->Class = static_cast<uint8_t>(config_revision >> 24);
            tpcidevice->device->cachelinesize = static_cast<uint8_t>(config_cacheline);
            tpcidevice->device->latencytimer = static_cast<uint8_t>(config_cacheline >> 8);
            tpcidevice->device->headertype = static_cast<uint8_t>(config_cacheline >> 16);
            tpcidevice->device->bist = static_cast<uint8_t>(config_cacheline >> 24);

            tpcidevice->vendorstr = getvendorname(tpcidevice->device->vendorid);
            tpcidevice->devicestr = getdevicename(tpcidevice->device->vendorid, tpcidevice->device->deviceid);
            tpcidevice->progifstr = getprogifname(tpcidevice->device->Class, tpcidevice->device->subclass, tpcidevice->device->progif);
            tpcidevice->subclassStr = getsubclassname(tpcidevice->device->Class, tpcidevice->device->subclass);
            tpcidevice->ClassStr = device_classes[tpcidevice->device->Class];

            devices.push_back(tpcidevice);

            log("%.4X:%.4X %s %s",
                devices.back()->device->vendorid,
                devices.back()->device->deviceid,
                devices.back()->vendorstr,
                devices.back()->devicestr);

            if (tpcidevice->device->Class == 0x06 && tpcidevice->device->subclass == 0x04)
            {
                enumbus(reinterpret_cast<pciheader1*>(tpcidevice->device)->secBus);
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
    if (!legacy && acpi::mcfghdr->header.length < sizeof(acpi::MCFGHeader) + sizeof(acpi::MCFGEntry))
    {
        error("No entries found in MCFG table!");
        return;
    }

    if (legacy)
    {
        if ((static_cast<uint8_t>(readl(0, 0, 0, 0, 0xC) >> 16) & 0x80) == 0) enumbus(0);
        else
        {
            for (size_t func = 0; func < 8; func++)
            {
                if (readl(0, 0, 0, func, 0) != 0xFFFF) break;
                enumbus(func);
            }
        }
    }
    else
    {
        size_t entries = ((acpi::mcfghdr->header.length) - sizeof(acpi::MCFGHeader)) / sizeof(acpi::MCFGEntry);
        for (size_t i = 0; i < entries; i++)
        {
            acpi::MCFGEntry &entry = acpi::mcfghdr->entries[i];
            for (uint64_t bus = entry.startbus; bus < entry.endbus; bus++)
            {
                enumbus(entry.baseaddr, bus, entry.segment);
            }
        }
    }

    serial::newline();
    initialised = true;
}
}