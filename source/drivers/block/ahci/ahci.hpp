// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/block/drivemgr/drivemgr.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/pci/pci.hpp>
#include <kernel/kernel.hpp>
#include <lib/lock.hpp>
#include <cstdint>

using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel::drivers::block::ahci {

enum status
{
    ATA_DEV_BUSY = 0x80,
    ATA_DEV_DRQ = 0x08
};

enum cmds
{
    ATA_CMD_READ_DMA_EX = 0x25,
    ATA_CMD_WRITE_DMA_EX = 0x35,
    ATA_CMD_IDENTIFY = 0xEC,

    ATAPI_CMD_PACKET = 0xA0,
    ATAPI_CMD_IDENTIFY = 0xEC,
    ATAPI_CMD_IDENTIFY_PACKET = 0xA1,
    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_CAPACITY = 0x25
};

enum hbaportstatus
{
    HBA_PxIS_TFES = (1 << 30),
    HBA_PORT_DEV_PRESENT = 0x03,
    HBA_PORT_IPM_ACTIVE = 0x01
};

enum types
{
    SATA_SIG_ATAPI = 0xEB140101,
    SATA_SIG_ATA = 0x00000101,
    SATA_SIG_SEMB = 0xC33C0101,
    SATA_SIG_PM = 0x96690101
};

enum hbacmd
{
    HBA_PxCMD_CR = 0x8000,
    HBA_PxCMD_FRE = 0x0010,
    HBA_PxCMD_ST = 0x0001,
    HBA_PxCMD_FR = 0x4000,
    HBA_PxCMD_ICC = (0xF << 28),
    HBA_PxCMD_ICC_ACTIVE = (1 << 28)
};

enum FIS_TYPE
{
    FIS_TYPE_REG_H2D = 0x27,
    FIS_TYPE_REG_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1
};

enum ghc
{
    AHCI_GHC_HR = (1 << 0),
    AHCI_GHC_IE = (1 << 1),
    AHCI_GHC_MRSM = (1 << 2),
    AHCI_GHC_AE = (1 << 31)
};

enum bohc
{
    AHCI_BIOS_BUSY = 1 << 4,
    AHCI_OWNER_OS = 1 << 1,
    AHCI_OWNER_BIOS = 1
};

enum ssts
{
    HBA_PxSSTS_DET = 0xFULL,
    HBA_PxSSTS_DET_INIT = 1,
    HBA_PxSSTS_DET_PRESENT = 3
};

enum sctls
{
    SCTL_PORT_DET_INIT 	 = 0x1,
	SCTL_PORT_IPM_NOPART = 0x100,
	SCTL_PORT_IPM_NOSLUM = 0x200,
	SCTL_PORT_IPM_NODSLP = 0x400
};

enum AHCIPortType
{
    NONE = 0,
    SATA = 1,
    SEMB = 2,
    PM = 3,
    SATAPI = 4
};

using HBAPort = volatile struct
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

using HBACommandHeader = volatile struct
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
    uint32_t InterruptOnCompletion : 1;
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

class AHCIPort : public drivemgr::Drive
{
    private:
    HBAPort *hbaport;
    uint8_t portNum;
    lock_t lock;

    void stopCMD();
    void startCMD();

    size_t findSlot();
    bool rw(uint64_t sector, uint32_t sectorCount, uint8_t *buffer, bool write);
    bool identify();

    public:
    bool initialised = false;
    AHCIPortType portType;

    int64_t read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
    {
        if (offset % this->stat.blksize || size % this->stat.blksize)
        {
            errno_set(EIO);
            return -1;
        }

        uint64_t start = offset / this->stat.blksize;
        uint64_t count = size / this->stat.blksize;
        uint8_t *abuffer = pmm::alloc<uint8_t*>(count) + hhdm_offset;

        if (!this->rw(start, count, abuffer, false))
        {
            errno_set(EIO);
            pmm::free(abuffer - hhdm_offset);
            return -1;
        }
        memcpy(buffer, abuffer, size);

        pmm::free(abuffer - hhdm_offset);
        return size;
    }

    int64_t write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
    {
        if (offset % this->stat.blksize || size % this->stat.blksize)
        {
            errno_set(EIO);
            return -1;
        }

        uint64_t start = offset / this->stat.blksize;
        uint64_t count = size / this->stat.blksize;
        uint8_t *abuffer = pmm::alloc<uint8_t*>(count) + hhdm_offset;

        memcpy(abuffer, buffer, size);
        if (!this->rw(start, count, abuffer, true))
        {
            errno_set(EIO);
            pmm::free(abuffer - hhdm_offset);
            return -1;
        }

        pmm::free(abuffer - hhdm_offset);
        return size;
    }

    int ioctl(void *handle, uint64_t request, void *argp)
    {
        return default_ioctl(this, request, argp);
    }

    void unref(void *handle)
    {
        this->refcount--;
    }

    void link(void *handle)
    {
        this->stat.nlink++;
    }

    void unlink(void *handle)
    {
        this->stat.nlink--;
    }

    void *mmap(uint64_t page, int flags)
    {
        return nullptr;
    }

    AHCIPort(HBAPort *hbaport, size_t portNum);
};

class AHCIController
{
    private:
    pci::pcidevice_t *pcidevice;
    HBAMemory *ABAR;

    public:
    bool initialised = false;
    vector<AHCIPort*> ports;

    AHCIController(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<AHCIController*> devices;

void init();
}