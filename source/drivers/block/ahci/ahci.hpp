// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/block/drivemgr/drivemgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::block::ahci {

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35

#define HBA_PxIS_TFES (1 << 30)

#define HBA_PORT_DEV_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE 0x1
#define SATA_SIG_ATAPI 0xEB140101
#define SATA_SIG_ATA 0x00000101
#define SATA_SIG_SEMB 0xC33C0101
#define SATA_SIG_PM 0x96690101

#define HBA_PxCMD_CR 0x8000
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FR 0x4000

#define HBA_PxCMD_CR 0x8000
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FR 0x4000

enum FIS_TYPE
{
    FIS_TYPE_REG_H2D = 0x27,
    FIS_TYPE_REG_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1,
};

enum AHCIPortType
{
    NONE = 0,
    SATA = 1,
    SEMB = 2,
    PM = 3,
    SATAPI = 4
};

struct HBAPort
{
    uint32_t CommandListBase;
    uint32_t CommandListBaseUpper;
    uint32_t FISBaseAddress;
    uint32_t FISBaseAddressUpper;
    uint32_t InterruptStatus;
    uint32_t InterruptEnable;
    uint32_t CommandStatus;
    uint32_t Reserved0;
    uint32_t TaskFileData;
    uint32_t Signature;
    uint32_t SataStatus;
    uint32_t SataControl;
    uint32_t SataError;
    uint32_t SataActive;
    uint32_t CommandIssue;
    uint32_t SataNotification;
    uint32_t FISSwitchControl;
    uint32_t Reserved1[11];
    uint32_t Vendor[4];
};

struct HBAMemory
{
    uint32_t HostCapability;
    uint32_t GlobalHostControl;
    uint32_t InterruptStatus;
    uint32_t PortsImplemented;
    uint32_t Version;
    uint32_t CCCControl;
    uint32_t CCCPorts;
    uint32_t EnclosureManagementLocation;
    uint32_t EnclosureManagementControl;
    uint32_t HostCapabilitiesExtended;
    uint32_t BIOSHandoffControlStatus;
    uint8_t Reserved0[0x74];
    uint8_t Vendor[0x60];
    HBAPort Ports[1];
};

struct HBACommandHeader
{
    uint8_t CommandFISLength : 5;
    uint8_t ATAPI : 1;
    uint8_t Write : 1;
    uint8_t Prefetchable : 1;
    uint8_t Reset : 1;
    uint8_t BIST : 1;
    uint8_t ClearBusy : 1;
    uint8_t Reserved0 : 1;
    uint8_t PortMultiplier : 4;
    uint16_t PRDTLength;
    uint32_t PRDBCount;
    uint32_t CommandTableBaseAddress;
    uint32_t CommandTableBaseAddressUpper;
    uint32_t Reserved1[4];
};

struct HBAPRDTEntry
{
    uint32_t DataBaseAddress;
    uint32_t DataBaseAddressUpper;
    uint32_t Reserved0;
    uint32_t ByteCount : 22;
    uint32_t Reserved1 : 9;
    uint32_t InterruptOnCompletion:1;
};

struct HBACommandTable
{
    uint8_t CommandFIS[64];
    uint8_t ATAPICommand[16];
    uint8_t Reserved[48];
    HBAPRDTEntry PRDTEntry[];
};

struct FIS_REG_H2D
{
    uint8_t FISType;
    uint8_t PortMultiplier : 4;
    uint8_t Reserved0 : 3;
    uint8_t CommandControl : 1;
    uint8_t Command;
    uint8_t FeatureLow;
    uint8_t LBA0;
    uint8_t LBA1;
    uint8_t LBA2;
    uint8_t DeviceRegister;
    uint8_t LBA3;
    uint8_t LBA4;
    uint8_t LBA5;
    uint8_t FeatureHigh;
    uint8_t CountLow;
    uint8_t CountHigh;
    uint8_t ISOCommandCompletion;
    uint8_t Control;
    uint8_t Reserved1[4];
};

struct FIS_REG_D2H
{
    uint8_t FISType;
    uint8_t PortMultiplier : 4;
    uint8_t Reserved0 : 2;
    uint8_t IntBit : 1;
    uint8_t Reserved1 : 1;
    uint8_t Status;
    uint8_t Error;
    uint8_t LBA0;
    uint8_t LBA1;
    uint8_t LBA2;
    uint8_t DeviceRegister;
    uint8_t LBA3;
    uint8_t LBA4;
    uint8_t LBA5;
    uint8_t Reserved2;
    uint8_t CountLow;
    uint8_t CountHigh;
    uint8_t Reserved3[2];
    uint8_t Reserved4[4];
};

struct FIS_DATA
{
    uint8_t FISType;
    uint8_t PortMultiplier : 4;
    uint8_t Reserved0 : 4;
    uint8_t Reserved1[2];
    uint32_t Data[1];
};

struct FIS_PIO_SETUP
{
    uint8_t FISType;
    uint8_t PortMultiplier : 4;
    uint8_t Reserved0 : 1;
    uint8_t DataDirection : 1;
    uint8_t IntBit : 1;
    uint8_t Reserved1 : 1;
    uint8_t Status;
    uint8_t Error;
    uint8_t LBA0;
    uint8_t LBA1;
    uint8_t LBA2;
    uint8_t DeviceRegister;
    uint8_t LBA3;
    uint8_t LBA4;
    uint8_t LBA5;
    uint8_t Reserved2;
    uint8_t CountLow;
    uint8_t CountHigh;
    uint8_t Reserved3;
    uint8_t EStatus;
    uint16_t TransferCount;
    uint8_t Reserved4[4];
};

struct FIS_DMA_SETUP
{
    uint8_t FISType;
    uint8_t PortMultiplier : 4;
    uint8_t Reserved0 : 1;
    uint8_t DataDirection : 1;
    uint8_t IntBit : 1;
    uint8_t AutoActivate : 1;
    uint8_t Reserved1[2];
    uint64_t DMABufferID;
    uint32_t Reserved2;
    uint32_t DMABufferOffset;
    uint32_t TransferCount;
    uint32_t Reserved3;
};

class AHCIDevice : public drivemgr::Drive
{
    private:
    lock_t lock;

    void stopCMD();
    void startCMD();

    size_t findSlot();
    bool rw(uint64_t sector, uint32_t sectorCount, uint16_t *buffer, bool write);

    public:
    HBAPort *hbaport;
    AHCIPortType portType;
    uint8_t portNum;

    void configure();
    
    bool read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer);
    bool write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer);
};

class AHCIController
{
    private:
    pci::pcidevice_t *pcidevice;
    HBAMemory *ABAR;

    public:
    bool initialised = false;
    AHCIDevice *ports[32];
    uint8_t portCount;

    void probePorts();
    AHCIController(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<AHCIController*> devices;

void init();
}