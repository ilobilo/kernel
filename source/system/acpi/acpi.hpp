// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <stdint.h>
#include <stddef.h>

namespace kernel::system::acpi {

#define ACPI_TIMER 0x0001
#define ACPI_BUSMASTER 0x0010
#define ACPI_GLOBAL 0x0020
#define ACPI_POWER_BUTTON 0x0100
#define ACPI_SLEEP_BUTTON 0x0200
#define ACPI_RTC_ALARM 0x0400
#define ACPI_PCIE_WAKE 0x4000
#define ACPI_WAKE 0x8000

#define ACPI_ENABLED 0x0001
#define ACPI_SLEEP 0x2000

#define ACPI_GAS_MMIO 0
#define ACPI_GAS_IO 1
#define ACPI_GAS_PCI 2

struct [[gnu::packed]] RSDP
{
    unsigned char signature[8];
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t revision;
    uint32_t rsdtaddr;
    uint32_t length;
    uint64_t xsdtaddr;
    uint8_t extchksum;
    uint8_t reserved[3];
};

struct [[gnu::packed]] SDTHeader
{
    unsigned char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t oemtableid[8];
    uint32_t oemrevision;
    uint32_t creatid;
    uint32_t creatrevision;
};

struct [[gnu::packed]] MCFGHeader
{
    SDTHeader header;
    uint64_t reserved;
};

struct [[gnu::packed]] MADTHeader
{
    SDTHeader sdt;
    uint32_t local_controller_addr;
    uint32_t flags;
    char entries_begin[];
};

struct [[gnu::packed]] MADT
{
    uint8_t type;
    uint8_t length;
};

struct [[gnu::packed]] MADTLapic
{
    MADT madtHeader;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
};

struct [[gnu::packed]] MADTIOApic
{
    MADT madtHeader;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t gsib;
};

struct [[gnu::packed]] MADTIso
{
    MADT madtHeader;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
};

struct [[gnu::packed]] MADTNmi
{
    MADT madtHeader;
    uint8_t processor;
    uint16_t flags;
    uint8_t lint;
};

struct [[gnu::packed]] GenericAddressStructure
{
    uint8_t AddressSpace;
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;
    uint64_t Address;
};

struct [[gnu::packed]] HPETHeader
{
    SDTHeader header;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    GenericAddressStructure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
};

struct [[gnu::packed]] FADTHeader
{
    SDTHeader header;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;
    uint8_t Reserved;
    uint8_t PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;
    uint16_t BootArchitectureFlags;
    uint8_t Reserved2;
    uint32_t Flags;
    GenericAddressStructure ResetReg;
    uint8_t ResetValue;
    uint8_t Reserved3[3];
    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt;
    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
};

struct [[gnu::packed]] deviceconfig
{
    uint64_t baseaddr;
    uint16_t pciseggroup;
    uint8_t startbus;
    uint8_t endbus;
    uint32_t reserved;
};

extern bool initialised;
extern bool madt;

extern bool use_xstd;
extern RSDP *rsdp;

extern MCFGHeader *mcfghdr;
extern MADTHeader *madthdr;
extern FADTHeader *fadthdr;
extern HPETHeader *hpethdr;
extern SDTHeader *rsdt;

extern Vector<MADTLapic*> lapics;
extern Vector<MADTIOApic*> ioapics;
extern Vector<MADTIso*> isos;
extern Vector<MADTNmi*> nmis;

extern uint32_t *SMI_CMD;
extern uint8_t ACPI_ENABLE;
extern uint8_t ACPI_DISABLE;
extern uint32_t PM1a_CNT;
extern uint32_t PM1b_CNT;
extern uint16_t SLP_TYPa;
extern uint16_t SLP_TYPb;
extern uint16_t SLP_EN;
extern uint16_t SCI_EN;
extern uint8_t  PM1_CNT_LEN;

extern uintptr_t lapic_addr;

void init();

void shutdown();
void reboot();

void *findtable(const char *signature, size_t skip = 0);
}