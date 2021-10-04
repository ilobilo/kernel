#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/pci/pcidesc.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>

void enumfunc(uint64_t deviceaddr, uint64_t func)
{
    uint64_t offset = func << 12;

    uint64_t funcaddr = deviceaddr + offset;

    pcideviceheader *pcidevice = (pcideviceheader*)funcaddr;

    if (pcidevice->deviceid == 0 || pcidevice->deviceid == 0xFFFF) return;

    serial_info("%s / %s / %s / %s / %s", getvendorname(pcidevice->vendorid),
        getdevicename(pcidevice->vendorid, pcidevice->deviceid),
        device_classes[pcidevice->Class],
        getsubclassname(pcidevice->Class, pcidevice->subclass),
        getprogifname(pcidevice->Class, pcidevice->subclass, pcidevice->progif));
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
    serial_info("Initializing PCI");

    int entries = ((mcfg->header.length) - sizeof(mcfg_header)) / sizeof(deviceconfig);
    for (int t = 0; t < entries; t++)
    {
        deviceconfig *newdevconf = (deviceconfig*)((uint64_t)mcfg + sizeof(mcfg_header) + (sizeof(deviceconfig)  *t));
        for (uint64_t bus = newdevconf->startbus; bus < newdevconf->endbus; bus++)
        {
            enumbus(newdevconf->baseaddr, bus);
        }
    }

    serial_info("Initialized PCI\n");
}
