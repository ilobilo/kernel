// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

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

uint32_t *SMI_CMD;
uint8_t ACPI_ENABLE;
uint8_t ACPI_DISABLE;
uint32_t PM1a_CNT;
uint32_t PM1b_CNT;
uint16_t SLP_TYPa;
uint16_t SLP_TYPb;
uint16_t SLP_EN;
uint16_t SCI_EN;
uint8_t PM1_CNT_LEN;

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

void dsdt_init()
{
    uint64_t dsdtaddr = ((is_canonical(fadthdr->X_Dsdt) && use_xstd) ? fadthdr->X_Dsdt : fadthdr->Dsdt);
    uint8_t *S5Addr = reinterpret_cast<uint8_t*>(dsdtaddr) + 36;
    uint64_t dsdtlength = reinterpret_cast<SDTHeader*>(dsdtaddr)->length;

    dsdtaddr *= 2;
    while (dsdtlength-- > 0)
    {
        if (!memcmp(S5Addr, "_S5_", 4)) break;
        S5Addr++;
    }

    if (dsdtlength <= 0)
    {
        error("_S5 not present in ACPI");
        error("ACPI shutdown may not be possible");
        return;
    }

    if ((*(S5Addr - 1) == 0x8 || (*(S5Addr - 2) == 0x8 && *(S5Addr - 1) == '\\')) && *(S5Addr + 4) == 0x12)
    {
        S5Addr += 5;
        S5Addr += ((*S5Addr & 0xC0) >> 6) + 2;

        if (*S5Addr == 0xA) S5Addr++;
        SLP_TYPb = *(S5Addr) << 10;
        SMI_CMD = reinterpret_cast<uint32_t*>(fadthdr->SMI_CommandPort);

        ACPI_ENABLE = fadthdr->AcpiEnable;
        ACPI_DISABLE = fadthdr->AcpiDisable;

        PM1a_CNT = fadthdr->PM1aControlBlock;
        PM1b_CNT = fadthdr->PM1bControlBlock;

        PM1_CNT_LEN = fadthdr->PM1ControlLength;

        SLP_EN = 1 << 13;
        SCI_EN = 1;
        return;
    }
    error("Failed to parse _S5 in ACPI");
    error("ACPI shutdown may not be possible");
    SCI_EN = 0;
}

void shutdown()
{
    if (SCI_EN == 1)
    {
        outw(fadthdr->PM1aControlBlock, (inw(fadthdr->PM1aControlBlock) & 0xE3FF) | ((SLP_TYPa << 10) | ACPI_SLEEP));
        if (fadthdr->PM1bControlBlock) outw(fadthdr->PM1bControlBlock, (inw(fadthdr->PM1bControlBlock) & 0xE3FF) | ((SLP_TYPb << 10) | ACPI_SLEEP));

        outw(PM1a_CNT, SLP_TYPa | SLP_EN);
        if (PM1b_CNT) outw(PM1b_CNT, SLP_TYPb | SLP_EN);
    }
}

void reboot()
{
    switch (fadthdr->ResetReg.AddressSpace)
    {
        case ACPI_GAS_MMIO:
            *reinterpret_cast<uint8_t*>(fadthdr->ResetReg.Address) = fadthdr->ResetValue;
            break;
        case ACPI_GAS_IO:
            outb(fadthdr->ResetReg.Address, fadthdr->ResetValue);
            break;
        case ACPI_GAS_PCI:
            pci::writeb(0, 0, (fadthdr->ResetReg.Address >> 32) & 0xFFFF, (fadthdr->ResetReg.Address >> 16) & 0xFFFF, fadthdr->ResetReg.Address & 0xFFFF, fadthdr->ResetValue);
            break;
    }
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

void init()
{
    log("Initialising ACPI");

    if (initialised)
    {
        warn("ACPI has already been initialised!\n");
        return;
    }

    rsdp = reinterpret_cast<RSDP*>(rsdp_tag->rsdp);

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

    outb(fadthdr->SMI_CommandPort, fadthdr->AcpiEnable);
    while (!(inw(fadthdr->PM1aControlBlock) & 1));

    if (madt) madt_init();
    dsdt_init();

    serial::newline();
    initialised = true;
}
}