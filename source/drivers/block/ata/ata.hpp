// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/block/drivemgr/drivemgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::block::ata {

#define ATA_REGISTER_DATA 0
#define ATA_REGISTER_ERROR 1
#define ATA_REGISTER_FEATURES 1
#define ATA_REGISTER_SECTOR_COUNT 2
#define ATA_REGISTER_LBA_LOW 3
#define ATA_REGISTER_LBA_MID 4
#define ATA_REGISTER_LBA_HIGH 5
#define ATA_REGISTER_DRIVE_HEAD 6
#define ATA_REGISTER_STATUS 7
#define ATA_REGISTER_COMMAND 7

#define ATA_BMR_CMD 0
#define ATA_BMR_DEV_SPECIFIC_1 1
#define ATA_BMR_STATUS 2
#define ATA_BMR_DEV_SPECIFIC_2 3
#define ATA_BMR_PRDT_ADDRESS 4
#define ATA_BMR_CMD_SECONDARY 8
#define ATA_BMR_DEV_SPECIFIC_1_SECONDARY 9
#define ATA_BMR_STATUS_SECONDARY 10
#define ATA_BMR_DEV_SPECIFIC_2_SECONDARY 11
#define ATA_BMR_PRDT_ADDRESS_SECONDARY 12

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRDY 0x40
#define ATA_DEV_DF 0x20
#define ATA_DEV_DSC 0x10
#define ATA_DEV_DRQ 0x08
#define ATA_DEV_CORR 0x04
#define ATA_DEV_IDX 0x02
#define ATA_DEV_ERR 0x01

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY 0xec

#define ATA_PRD_BUFFER(x) ((x) & 0xFFFFFFFF)
#define ATA_PRD_TRANSFER_SIZE(x) (((x) & 0xFFFFULL) << 32)
#define ATA_PRD_END 0x8000000000000000ULL

#define ATA_PRIMARY_IRQ 14
#define ATA_SECONDARY_IRQ 15

struct ATAController;
class ATADevice : public drivemgr::Drive
{
    private:
    ATAController *parent;

    lock_t lock;
    size_t port;
    size_t drive;

    uint64_t *prdt;

    bool rw(uint64_t sector, uint32_t sectorCount, uint16_t *buffer, bool write);

    public:
    bool read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer);
    bool write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer);

    ATADevice(size_t port, size_t drive, ATAController *parent);
};

struct ATAController
{
    pci::pcidevice_t *pcidevice;

    vector<ATADevice*> drives;

    int port0 = 0x1F0;
    int port1 = 0x170;
    int ctrlport0 = 0x3F6;
    int ctrlport1 = 0x376;

    uint32_t bmport;

    void outreg(uint8_t port, uint8_t reg, uint8_t val);
    uint8_t inreg(uint8_t port, uint8_t reg);

    void outctrlreg(uint8_t port, uint8_t reg, uint8_t val);
    uint8_t inctrlreg(uint8_t port, uint8_t reg);

    bool detect(size_t port, size_t drive);

    bool initialised = false;

    ATAController(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<ATAController*> devices;

void init();
}