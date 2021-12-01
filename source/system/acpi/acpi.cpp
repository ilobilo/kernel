// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/mmio.hpp>
#include <lib/io.hpp>
#include <lai/host.h>
#include <lai/core.h>
#include <main.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

namespace kernel::system::acpi {

bool initialised = false;
bool madt = false;

bool use_xstd;
RSDP *rsdp;

MCFGHeader *mcfghdr;
MADTHeader *madthdr;
FADTHeader *fadthdr;
HPETHeader *hpethdr;
SDTHeader *rsdt;

Vector<MADTLapic*> lapics;
Vector<MADTIOApic*> ioapics;
Vector<MADTIso*> isos;
Vector<MADTNmi*> nmis;

uintptr_t lapic_addr = 0;

void init()
{
    serial::info("Initialising ACPI");

    if (initialised)
    {
        serial::warn("ACPI has already been initialised!\n");
        return;
    }

    rsdp = (RSDP*)rsdp_tag->rsdp;

    if (rsdp->revision >= 2 && rsdp->xsdtaddr)
    {
        use_xstd = true;
        rsdt = (SDTHeader*)rsdp->xsdtaddr;
        serial::info("Found XSDT at: 0x%X", rsdp->xsdtaddr);
    }
    else
    {
        use_xstd = false;
        rsdt = (SDTHeader*)((uintptr_t)rsdp->rsdtaddr);
        serial::info("Found RSDT at: 0x%X", rsdp->rsdtaddr);
    }

    mcfghdr = (MCFGHeader*)findtable("MCFG", 0);
    madthdr = (MADTHeader*)findtable("APIC", 0);
    if (madthdr) madt = true;
    fadthdr = (FADTHeader*)findtable("FACP", 0);
    hpethdr = (HPETHeader*)findtable("HPET", 0);

    lai_set_acpi_revision(rsdp->revision);
    lai_create_namespace();

    if (madt)
    {
        serial::newline();

        lapics.init(1);
        ioapics.init(1);
        isos.init(1);
        nmis.init(1);

        lapic_addr = madthdr->local_controller_addr;
        
        for (uint8_t *madt_ptr = (uint8_t*)madthdr->entries_begin; (uintptr_t)madt_ptr < (uintptr_t)madthdr + madthdr->sdt.length; madt_ptr += *(madt_ptr + 1))
        {
            switch (*(madt_ptr))
            {
                case 0:
                    serial::info("ACPI/MADT: Found local APIC %ld", lapics.size());
                    lapics.push_back((MADTLapic*)madt_ptr);
                    break;
                case 1:
                    serial::info("ACPI/MADT: Found I/O APIC %ld", ioapics.size());
                    ioapics.push_back((MADTIOApic*)madt_ptr);
                    break;
                case 2:
                    serial::info("ACPI/MADT: Found ISO %ld", isos.size());
                    isos.push_back((MADTIso*)madt_ptr);
                    break;
                case 4:
                    serial::info("ACPI/MADT: Found NMI %ld", nmis.size());
                    nmis.push_back((MADTNmi*)madt_ptr);
                    break;
                case 5:
                    lapic_addr = QWORD_PTR(madt_ptr + 4);
                    break;
            }
        }
    }

    serial::newline();
    initialised = true;
}

void *findtable(const char *signature, size_t skip)
{
    if (skip < 0) skip = 0;
    size_t entries = (rsdt->length - sizeof(SDTHeader)) / (use_xstd ? 8 : 4);
    for (size_t i = 0; i < entries; i++)
    {
        SDTHeader *newsdthdr;
        if (use_xstd) newsdthdr = (SDTHeader*)*(uint64_t*)((uint64_t)rsdt + sizeof(SDTHeader) + (i * 8));
        else newsdthdr = (SDTHeader*)((uintptr_t)*(uint32_t*)((uint32_t)((uintptr_t)rsdt) + sizeof(SDTHeader) + (i * 4)));
        
        if (!newsdthdr || !strcmp((const char*)newsdthdr->signature, "")) continue;

        if (!strncmp((const char*)newsdthdr->signature, signature, 4))
        {
            if (!skip) return newsdthdr;
            else skip--;
        }
        else continue;
    }
    return 0;
}
}

void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            serial::info("%s", msg);
            break;
        case LAI_WARN_LOG:
            serial::warn("%s", msg);
            break;
    }
}

__attribute__((noreturn)) void laihost_panic(const char *msg)
{
    serial::err("%s", msg);
    while (true) asm volatile ("cli; hlt");
}

void *laihost_malloc(size_t size)
{
    return heap::malloc(size);
}

void *laihost_realloc(void *ptr, size_t size, size_t oldsize)
{
    return heap::realloc(ptr, size);
}

void laihost_free(void *ptr, size_t oldsize)
{
    heap::free(ptr);
}

void *laihost_map(size_t address, size_t size)
{
	return (void*)address;
}

void laihost_unmap(void *pointer, size_t count)
{
}

__attribute__((always_inline)) inline bool is_canonical(uint64_t addr)
{
    return ((addr <= 0x00007FFFFFFFFFFF) || ((addr >= 0xFFFF800000000000) && (addr <= 0xFFFFFFFFFFFFFFFF)));
}

void *laihost_scan(const char *signature, size_t index)
{
	if (!strncmp(signature, "DSDT", 4))
    {
		uint64_t dsdt_addr = 0;

		if (is_canonical(kernel::system::acpi::fadthdr->X_Dsdt) && kernel::system::acpi::use_xstd) dsdt_addr = kernel::system::acpi::fadthdr->X_Dsdt;
		else dsdt_addr = kernel::system::acpi::fadthdr->Dsdt;

		return (void*)dsdt_addr;
    }
    else return kernel::system::acpi::findtable(signature, index);
}

void laihost_outb(uint16_t port, uint8_t val)
{
    outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val)
{
    outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val)
{
    outl(port, val);
}

uint8_t laihost_inb(uint16_t port)
{
    return inb(port);
}

uint16_t laihost_inw(uint16_t port)
{
    return inw(port);
}

uint32_t laihost_ind(uint16_t port)
{
    return inl(port);
}


void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val)
{
    kernel::system::pci::writeb(bus, slot, fun, offset, val);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val)
{
    kernel::system::pci::writew(bus, slot, fun, offset, val);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val)
{
    kernel::system::pci::writel(bus, slot, fun, offset, val);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return kernel::system::pci::readb(bus, slot, fun, offset);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return kernel::system::pci::readw(bus, slot, fun, offset);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return kernel::system::pci::readl(bus, slot, fun, offset);
}

void laihost_sleep(uint64_t ms)
{
    kernel::system::sched::hpet::usleep(ms * 1000);
}

uint64_t laihost_timer()
{
    return kernel::system::sched::hpet::counter() / 100000000;
}