// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/block/drivemgr/drivemgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::block::ata {

enum regs
{
    ATA_REGISTER_DATA = 0x00,
    ATA_REGISTER_ERROR = 0x01,
    ATA_REGISTER_FEATURES = 0x01,
    ATA_REGISTER_SECTOR_COUNT = 0x02,
    ATA_REGISTER_LBA_LOW = 0x03,
    ATA_REGISTER_LBA_MID = 0x04,
    ATA_REGISTER_LBA_HIGH = 0x05,
    ATA_REGISTER_DRIVE_HEAD = 0x06,
    ATA_REGISTER_STATUS = 0x07,
    ATA_REGISTER_COMMAND = 0x07
};

enum idents
{
    ATA_IDENT_DEVICETYPE = 0,
    ATA_IDENT_CYLINDERS = 1,
    ATA_IDENT_HEADS = 3,
    ATA_IDENT_SECTORS = 6,
    ATA_IDENT_SERIAL = 10,
    ATA_IDENT_MODEL = 27,
    ATA_IDENT_CAPABILITIES = 49,
    ATA_IDENT_FIELDVALID = 53,
    ATA_IDENT_MAX_LBA = 160,
    ATA_IDENT_COMMANDSETS = 82,
    ATA_IDENT_MAX_LBA_EXT = 100
};

enum bmrcmd
{
    ATA_BMR_CMD = 0x00,
    ATA_BMR_DEV_SPECIFIC_1 = 0x01,
    ATA_BMR_STATUS = 0x02,
    ATA_BMR_DEV_SPECIFIC_2 = 0x03,
    ATA_BMR_PRDT_ADDRESS = 0x04,
    ATA_BMR_CMD_SECONDARY = 0x08,
    ATA_BMR_DEV_SPECIFIC_1_SECONDARY = 0x09,
    ATA_BMR_STATUS_SECONDARY = 0x0A,
    ATA_BMR_DEV_SPECIFIC_2_SECONDARY = 0x0B,
    ATA_BMR_PRDT_ADDRESS_SECONDARY = 0x0C
};

enum status
{
    ATA_DEV_BUSY = 0x80,
    ATA_DEV_DRDY = 0x40,
    ATA_DEV_DF = 0x20,
    ATA_DEV_DSC = 0x10,
    ATA_DEV_DRQ = 0x08,
    ATA_DEV_CORR = 0x04,
    ATA_DEV_IDX = 0x02,
    ATA_DEV_ERR = 0x01
};

enum cmds
{
    ATA_CMD_READ_DMA_EX = 0x25,
    ATA_CMD_WRITE_DMA_EX = 0x35,
    ATA_CMD_IDENTIFY = 0xEC,
    ATAPI_PACKET = 0xA0,
    ATAPI_IDENTIFY = 0xEC,
    ATAPI_IDENTIFY_PACKET = 0xA1,
    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_EJECT = 0x1B
};

enum ATAPortType
{
    ATA = 0,
    ATAPI = 1
};

struct ATAController;
class ATAPort : public drivemgr::Drive
{
    private:
    lock_t lock;
    uint16_t port;
    uint16_t bmport;
    size_t drive;

    uint64_t *prdt;
    uint64_t *prdtBuffer;

    void outbcmd(uint8_t offset, uint8_t val);
    void outwcmd(uint8_t offset, uint16_t val);
    void outlcmd(uint8_t offset, uint32_t val);

    uint8_t inbcmd(uint8_t offset);
    uint16_t inwcmd(uint8_t offset);
    uint32_t inlcmd(uint8_t offset);

    bool rw(uint64_t sector, uint32_t sectorCount, bool write);

    public:
    bool initialised = false;

    ATAPortType portType;

    bool read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer);
    bool write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer);

    ATAPort(uint16_t port, uint16_t bmport, size_t drive);
};

struct ATAController
{
    pci::pcidevice_t *pcidevice;

    vector<ATAPort*> ports;

    uint16_t port[2] = { 0x1F0, 0x170 };
    uint16_t ctrlport[2] = { 0x3F6, 0x376 };

    uint16_t bmport;

    bool initialised = false;

    ATAController(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<ATAController*> devices;

void init();
}