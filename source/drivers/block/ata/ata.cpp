// Copyright (C) 2021-2022  ilobilo

#include <drivers/block/ata/ata.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::system::mm;

namespace kernel::drivers::block::ata {

bool initialised = false;
vector<ATAController*> devices;

void ATAPort::outbcmd(uint8_t offset, uint8_t val)
{
    outb(this->port + offset, val);
}
void ATAPort::outwcmd(uint8_t offset, uint16_t val)
{
    outw(this->port + offset, val);
}
void ATAPort::outlcmd(uint8_t offset, uint32_t val)
{
    outl(this->port + offset, val);
}

uint8_t ATAPort::inbcmd(uint8_t offset)
{
    return inb(this->port + offset);
}
uint16_t ATAPort::inwcmd(uint8_t offset)
{
    return inw(this->port + offset);
}
uint32_t ATAPort::inlcmd(uint8_t offset)
{
    return inl(this->port + offset);
}

bool ATAPort::rw(uint64_t sector, uint32_t sectorCount, bool write)
{
    if (this->initialised == false) return false;
    lockit(this->lock);

    outb(this->bmport + ATA_BMR_CMD, 0);
    outl(this->bmport + ATA_BMR_PRDT_ADDRESS, static_cast<uint32_t>(reinterpret_cast<uint64_t>(this->prdt)));
    outb(this->bmport + ATA_BMR_STATUS, inb(this->bmport + ATA_BMR_STATUS) | 0x04 | 0x02);

    this->outbcmd(ATA_REGISTER_DRIVE_HEAD, 0x40 | (this->drive << 4));

    for (size_t i = 0; i < 4; i++) inb(this->ctrlport0);

    while (this->inbcmd(ATA_REGISTER_STATUS) & ATA_DEV_BUSY);

    this->outbcmd(ATA_REGISTER_SECTOR_COUNT, (sectorCount >> 8) & 0xFF);

    this->outbcmd(ATA_REGISTER_LBA_LOW, (sector >> 24) & 0xFF);
    this->outbcmd(ATA_REGISTER_LBA_MID, (sector >> 32) & 0xFF);
    this->outbcmd(ATA_REGISTER_LBA_HIGH, (sector >> 40) & 0xFF);

    for (size_t i = 0; i < 4; i++) inb(this->ctrlport0);

    this->outbcmd(ATA_REGISTER_SECTOR_COUNT, sectorCount & 0xFF);

    this->outbcmd(ATA_REGISTER_LBA_LOW, sector & 0xFF);
    this->outbcmd(ATA_REGISTER_LBA_MID, (sector >> 8) & 0xFF);
    this->outbcmd(ATA_REGISTER_LBA_HIGH, (sector >> 16) & 0xFF);

    for (size_t i = 0; i < 4; i++) inb(this->ctrlport0);

    while (this->inbcmd(ATA_REGISTER_STATUS) & ATA_DEV_BUSY || !(this->inbcmd(ATA_REGISTER_STATUS) & ATA_DEV_DRDY));

    this->outbcmd(ATA_REGISTER_COMMAND, (write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX));
    outb(this->bmport + ATA_BMR_CMD, (write ? 0x00 : 0x08) | 0x01);

    uint8_t status = this->inbcmd(ATA_REGISTER_STATUS);
    while (status & ATA_DEV_BUSY)
    {
        if (status & ATA_DEV_ERR)
        {
            error("ATA: %s error!", write ? "write" : "read");
            return false;
        }
        status = this->inbcmd(ATA_REGISTER_STATUS);
    }

    outb(this->bmport + ATA_BMR_CMD, 0);

    if (this->inbcmd(ATA_REGISTER_STATUS) & ATA_DEV_ERR)
    {
        error("ATA: %s error 0x%X!", write ? "write" : "read", this->inbcmd(ATA_REGISTER_ERROR));
        return false;
    }

    return true;
}

bool ATAPort::read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    if (sector + sectorCount > this->sectors) sectorCount = this->sectors = sector;
    for (size_t i = 0; i < sectorCount; i++)
    {
        if (!this->rw(sector + i, 1, false)) return false;
        memcpy(buffer, this->prdtBuffer, 512);
        buffer += 512;
    }
    return true;
}
bool ATAPort::write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    if (sector + sectorCount > this->sectors) sectorCount = this->sectors = sector;
    for (size_t i = 0; i < sectorCount; i++)
    {
        memcpy(this->prdtBuffer, buffer, 512);
        if (!this->rw(sector + i, 1, true)) return true;
        buffer += 512;
    }
    return true;
}

ATAPort::ATAPort(uint16_t port, uint16_t bmport, uint16_t ctrlport0, size_t drive)
{
    this->port = port;
    this->drive = drive;
    this->bmport = bmport;
    this->ctrlport0 = ctrlport0;

    this->outbcmd(ATA_REGISTER_DRIVE_HEAD, 0xA0 | (drive << 4));

    for (size_t i = 0; i < 4; i++) inb(this->ctrlport0);

    this->outbcmd(ATA_REGISTER_SECTOR_COUNT, 0);
    this->outbcmd(ATA_REGISTER_LBA_LOW, 0);
    this->outbcmd(ATA_REGISTER_LBA_MID, 0);
    this->outbcmd(ATA_REGISTER_LBA_HIGH, 0);

    this->outbcmd(ATA_REGISTER_COMMAND, ATA_CMD_IDENTIFY);

    if (!this->inbcmd(ATA_REGISTER_STATUS)) return;

    size_t timer = 100000;
    while (timer--)
    {
        uint8_t status = this->inbcmd(ATA_REGISTER_STATUS);
        if (status & ATA_DEV_ERR) return;
        if (!(status & ATA_DEV_BUSY) && (status & ATA_DEV_DRQ)) break;
    }
    if (timer <= 0)
    {
        error("ATA: Port #%zu is not answering!", port);
        return;
    }

    if (this->inbcmd(ATA_REGISTER_LBA_MID) || this->inbcmd(ATA_REGISTER_LBA_HIGH))
    {
        error("ATA: Port #%zu is non-standard ATAPI!", port);
        return;
    }

    timer = 100000;
    while (timer--)
    {
        uint8_t status = this->inbcmd(ATA_REGISTER_STATUS);
        if (status & ATA_DEV_ERR)
        {
            error("ATA: Port #%zu error!", port);
            return;
        }
        if (status & ATA_DEV_DRQ) break;
    }
    if (timer <= 0)
    {
        error("ATA: Port #%zu is hung!", port);
        return;
    }

    uint16_t identify[256];
    for (size_t i = 0; i < 256; i++) identify[i] = this->inwcmd(ATA_REGISTER_DATA);

    this->portType = ATA;

    if (static_cast<uint32_t>(identify[ATA_IDENT_COMMANDSETS] | (identify[ATA_IDENT_COMMANDSETS + 1] << 16)) & (1 << 26))
    {
        this->sectors = static_cast<uint32_t>(identify[ATA_IDENT_MAX_LBA_EXT] | (identify[ATA_IDENT_MAX_LBA_EXT + 1] << 16));
        this->size = static_cast<uint32_t>(identify[ATA_IDENT_MAX_LBA_EXT] | (identify[ATA_IDENT_MAX_LBA_EXT + 1] << 16)) * 512;
    }
    else
    {
        this->sectors = static_cast<uint32_t>(identify[ATA_IDENT_MAX_LBA] | (identify[ATA_IDENT_MAX_LBA + 1] << 16));
        this->size = static_cast<uint32_t>(identify[ATA_IDENT_MAX_LBA] | (identify[ATA_IDENT_MAX_LBA + 1] << 16)) * 512;
    }

    this->buffer = static_cast<uint8_t*>(pmm::alloc());
    this->prdt = static_cast<uint64_t*>(pmm::alloc());
    this->prdtBuffer = static_cast<uint64_t*>(pmm::alloc());

    *this->prdt = (reinterpret_cast<uint64_t>(this->prdtBuffer) | (static_cast<uint64_t>(0x1000) << 32) | 0x8000000000000000ULL) & 0xFFFFFFFF;

    this->initialised = true;
}

ATAController::ATAController(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering ATA driver #%zu", devices.size());

    pci::pcibar bar = this->pcidevice->get_bar(0);
    if (bar.address != 0 && !bar.mmio) this->port[0] = bar.address;

    bar = this->pcidevice->get_bar(1);
    if (bar.address != 0 && !bar.mmio) this->port[1] = bar.address;

    bar = this->pcidevice->get_bar(2);
    if (bar.address != 0 && !bar.mmio) this->ctrlport[0] = bar.address;

    bar = this->pcidevice->get_bar(3);
    if (bar.address != 0 && !bar.mmio) this->ctrlport[1] = bar.address;

    bar = this->pcidevice->get_bar(4);
    if (bar.address == 0 || bar.mmio) return;
    this->bmport = bar.address;

    this->pcidevice->command(pci::CMD_BUS_MAST, true);

    for (size_t i = 0; i < 2; i++)
    {
        outb(this->ctrlport[i], inb(this->ctrlport[i]) | 0x04);
        for (size_t t = 0; t < 4; t++) inb(this->ctrlport[0]);
        outb(this->ctrlport[i], 0x00);

        for (size_t t = 0; t < 2; t++)
        {
            this->ports.push_back(new ATAPort((i ? this->port[1] : this->port[0]), this->bmport, this->ctrlport[0], t));
            if (this->ports.back()->initialised == false)
            {
                free(this->ports.back());
                this->ports.pop_back();
            }
        }
    }

    if (this->ports.size() == 0) return;
    this->initialised = true;
}

uint16_t ids[8][3] = {
    { 0x01, 0x01, 0x00 },
    { 0x01, 0x01, 0x05 },
    { 0x01, 0x01, 0x0A },
    { 0x01, 0x01, 0x0F },
    { 0x01, 0x01, 0x80 },
    { 0x01, 0x01, 0x85 },
    { 0x01, 0x01, 0x8A },
    { 0x01, 0x01, 0x8F },
};

bool search(uint8_t Class, uint8_t subclass, uint8_t progif)
{
    size_t count = pci::count(Class, subclass, progif);
    if (count == 0) return false;

    for (size_t i = 0; i < count; i++)
    {
        devices.push_back(new ATAController(pci::search(Class, subclass, progif, i)));
        if (devices.back()->initialised == false)
        {
            free(devices.back());
            devices.pop_back();
            error("Could not initialise ATA driver #%zu", devices.size());
        }
    }
    return true;
}

void init()
{
    log("Initialising ATA driver");

    if (initialised)
    {
        warn("ATA driver has already been initialised!\n");
        return;
    }

    bool found[8] = { [0 ... 7] = false };
    for (size_t i = 0; i < 8; i++)
    {
        found[i] = search(ids[i][0], ids[i][1], ids[i][2]);
    }
    for (size_t i = 0; i < 8; i++)
    {
        if (found[i] == true)
        {
            serial::newline();
            if (devices.size() != 0) initialised = true;
            return;
        }
    }
    error("No ATA cards found!\n");
}
}