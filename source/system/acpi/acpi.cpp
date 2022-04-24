// Copyright (C) 2021-2022  ilobilo

#include <system/sched/hpet/hpet.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/acpi/acpi.hpp>
#include <drivers/ps2/ps2.hpp>
#include <system/pci/pci.hpp>
#include <kernel/kernel.hpp>
#include <acpispec/tables.h>
#include <lai/helpers/pm.h>
#include <lib/timer.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>
#include <lai/host.h>
#include <lai/core.h>
#include <lib/io.hpp>

using namespace kernel::system::sched;
using namespace kernel::system::mm;
using namespace kernel::drivers;

[[gnu::always_inline]] inline bool is_canonical(uint64_t addr)
{
    return ((addr <= 0x00007FFFFFFFFFFF) || ((addr >= 0xFFFF800000000000) && (addr <= 0xFFFFFFFFFFFFFFFF)));
}

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

vector<MADTLapic*> lapics;
vector<MADTIOApic*> ioapics;
vector<MADTIso*> isos;
vector<MADTNmi*> nmis;

uintptr_t lapic_addr = 0;

void madt_init()
{
    serial::newline();

    lapic_addr = madthdr->local_controller_addr;

    for (uint8_t *madt_ptr = reinterpret_cast<uint8_t*>(madthdr->entries_begin); reinterpret_cast<uintptr_t>(madt_ptr) < reinterpret_cast<uintptr_t>(madthdr) + madthdr->sdt.length; madt_ptr += *(madt_ptr + 1))
    {
        switch (*(madt_ptr))
        {
            case 0:
                log("ACPI/MADT: Found local APIC %ld", lapics.size());
                lapics.push_back(reinterpret_cast<MADTLapic*>(madt_ptr));
                break;
            case 1:
                log("ACPI/MADT: Found I/O APIC %ld", ioapics.size());
                ioapics.push_back(reinterpret_cast<MADTIOApic*>(madt_ptr));
                break;
            case 2:
                log("ACPI/MADT: Found ISO %ld", isos.size());
                isos.push_back(reinterpret_cast<MADTIso*>(madt_ptr));
                break;
            case 4:
                log("ACPI/MADT: Found NMI %ld", nmis.size());
                nmis.push_back(reinterpret_cast<MADTNmi*>(madt_ptr));
                break;
            case 5:
                lapic_addr = QWORD_PTR(madt_ptr + 4);
                break;
        }
    }
}

void shutdown()
{
    lai_enter_sleep(5);
}

void reboot()
{
    lai_acpi_reset();
    ps2::reboot();
}

void *findtable(const char *signature, size_t skip)
{
    if (skip < 0) skip = 0;
    size_t entries = (rsdt->length - sizeof(SDTHeader)) / (use_xstd ? 8 : 4);

    for (size_t i = 0; i < entries; i++)
    {
        SDTHeader *newsdthdr;
        if (use_xstd) newsdthdr = reinterpret_cast<SDTHeader*>(*reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(rsdt) + sizeof(SDTHeader) + (i * 8)));
        else newsdthdr = reinterpret_cast<SDTHeader*>(*reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(rsdt) + sizeof(SDTHeader) + (i * 4)));

        if (!newsdthdr || !strcmp(reinterpret_cast<const char*>(newsdthdr->signature), "")) continue;

        if (!strncmp(reinterpret_cast<const char*>(newsdthdr->signature), signature, 4))
        {
            if (skip == 0) return newsdthdr;
            else skip--;
        }
    }
    return nullptr;
}

void init()
{
    log("Initialising ACPI");

    if (initialised)
    {
        warn("ACPI has already been initialised!\n");
        return;
    }

    rsdp = reinterpret_cast<RSDP*>(rsdp_request.response->address);

    if (rsdp->revision >= 2 && rsdp->xsdtaddr)
    {
        use_xstd = true;
        rsdt = reinterpret_cast<SDTHeader*>(rsdp->xsdtaddr);
        log("Found XSDT at: 0x%X", rsdp->xsdtaddr);
    }
    else
    {
        use_xstd = false;
        rsdt = reinterpret_cast<SDTHeader*>(rsdp->rsdtaddr);
        log("Found RSDT at: 0x%X", rsdp->rsdtaddr);
    }

    mcfghdr = reinterpret_cast<MCFGHeader*>(findtable("MCFG", 0));
    madthdr = reinterpret_cast<MADTHeader*>(findtable("APIC", 0));
    if (madthdr) madt = true;
    fadthdr = reinterpret_cast<FADTHeader*>(findtable("FACP", 0));
    hpethdr = reinterpret_cast<HPETHeader*>(findtable("HPET", 0));

    if (madt) madt_init();

    lai_set_acpi_revision(rsdp->revision);
    lai_create_namespace();

    serial::newline();
    initialised = true;
}
}

void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            log("%s", msg);
            break;
        case LAI_WARN_LOG:
            warn("%s", msg);
            break;
    }
}

__attribute__((noreturn)) void laihost_panic(const char *msg)
{
    error("%s", msg);
    while (true) asm volatile ("cli; hlt");
}

void *laihost_malloc(size_t size)
{
    return malloc(size);
}

void *laihost_realloc(void *ptr, size_t size, size_t oldsize)
{
    return realloc(ptr, size);
}

void laihost_free(void *ptr, size_t oldsize)
{
    free(ptr);
}

void *laihost_map(size_t address, size_t count)
{
    for (size_t i = 0; i < count; i += 0x1000)
    {
        vmm::kernel_pagemap->mapMem(address + hhdm_offset, address);
    }
	return reinterpret_cast<void*>(address + hhdm_offset);
}

void laihost_unmap(void *address, size_t count)
{
    for (size_t i = 0; i < count; i += 0x1000)
    {
        vmm::kernel_pagemap->unmapMem(reinterpret_cast<uint64_t>(address) + i);
    }
}

void *laihost_scan(const char *signature, size_t index)
{
	if (!strncmp(signature, "DSDT", 4))
    {
		uint64_t dsdt_addr = 0;

		if (is_canonical(kernel::system::acpi::fadthdr->X_Dsdt) && kernel::system::acpi::use_xstd) dsdt_addr = kernel::system::acpi::fadthdr->X_Dsdt;
		else dsdt_addr = kernel::system::acpi::fadthdr->Dsdt;

		return reinterpret_cast<void*>(dsdt_addr);
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
    kernel::system::pci::writeb(seg, bus, slot, fun, offset, val);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val)
{
    kernel::system::pci::writew(seg, bus, slot, fun, offset, val);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val)
{
    kernel::system::pci::writel(seg, bus, slot, fun, offset, val);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return kernel::system::pci::readb(seg, bus, slot, fun, offset);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return kernel::system::pci::readw(seg, bus, slot, fun, offset);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return kernel::system::pci::readl(seg, bus, slot, fun, offset);
}

void laihost_sleep(uint64_t ms)
{
    timer::usleep(ms * 1000);
}

uint64_t laihost_timer()
{
    return kernel::system::sched::hpet::counter() / 100000000;
}