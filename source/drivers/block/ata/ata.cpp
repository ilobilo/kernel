// Copyright (C) 2021-2022  ilobilo

#include <system/sched/timer/timer.hpp>
#include <drivers/block/ata/ata.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::system::sched;
using namespace kernel::system::mm;

namespace kernel::drivers::block::ata {

bool initialised = false;
vector<ATAController*> devices;

[[clang::optnone]] bool ATADevice::rw(uint64_t sector, uint32_t sectorCount, uint16_t *buffer, bool write)
{
    this->lock.lock();
    *this->prdt = ATA_PRD_BUFFER(reinterpret_cast<uint64_t>(buffer) | (static_cast<uint64_t>(0x1000) << 32) | ATA_PRD_END);

    outb(this->parent->bmport + ATA_BMR_CMD, 0);
    outl(this->parent->bmport + ATA_BMR_PRDT_ADDRESS, static_cast<uint32_t>(reinterpret_cast<uint64_t>(this->prdt)));
    outb(this->parent->bmport + ATA_BMR_STATUS, inb(this->parent->bmport + ATA_BMR_STATUS) | 0x04 | 0x02);

    this->parent->outreg(this->port, ATA_REGISTER_DRIVE_HEAD, 0x40 | (this->drive << 4));
    for (size_t i = 0; i < 4; i++) inb(this->parent->ctrlport0);

    while (this->parent->inreg(this->port, ATA_REGISTER_STATUS) & ATA_DEV_BUSY);

    this->parent->outreg(this->port, ATA_REGISTER_SECTOR_COUNT, (sectorCount >> 8) & 0xFF);

    this->parent->outreg(this->port, ATA_REGISTER_LBA_LOW, (sector >> 24) & 0xFF);
    this->parent->outreg(this->port, ATA_REGISTER_LBA_MID, (sector >> 32) & 0xFF);
    this->parent->outreg(this->port, ATA_REGISTER_LBA_HIGH, (sector >> 40) & 0xFF);

    for (size_t i = 0; i < 4; i++) inb(this->parent->ctrlport0);

    this->parent->outreg(this->port, ATA_REGISTER_SECTOR_COUNT, sectorCount & 0xFF);

    this->parent->outreg(this->port, ATA_REGISTER_LBA_LOW, sector & 0xFF);
    this->parent->outreg(this->port, ATA_REGISTER_LBA_MID, (sector >> 8) & 0xFF);
    this->parent->outreg(this->port, ATA_REGISTER_LBA_HIGH, (sector >> 16) & 0xFF);

    for (size_t i = 0; i < 4; i++) inb(this->parent->ctrlport0);

    while (this->parent->inreg(this->port, ATA_REGISTER_STATUS) & ATA_DEV_BUSY || !(this->parent->inreg(this->port, ATA_REGISTER_STATUS) & ATA_DEV_DRDY));

    this->parent->outreg(this->port, ATA_REGISTER_COMMAND, (write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX));
    outb(this->parent->bmport + ATA_BMR_CMD, (write ? 0x00 : 0x08) | 0x01);

    uint8_t status = this->parent->inreg(this->port, ATA_REGISTER_STATUS);
    while (status & ATA_DEV_BUSY)
    {
        if (status & ATA_DEV_ERR) break;
        status = this->parent->inreg(this->port, ATA_REGISTER_STATUS);
    }

    outb(this->parent->bmport + ATA_BMR_CMD, 0);
    if (this->parent->inreg(this->port, ATA_REGISTER_STATUS) & 0x01)
    {
        error("ATA: %s error 0x%X!", (write) ? "write" : "read", this->parent->inreg(this->port, ATA_REGISTER_ERROR));
        this->lock.unlock();
        return false;
    }

    this->lock.unlock();
    return true;
}

bool ATADevice::read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    // for (size_t i = 0; i < sectorCount; i++)
    // {
    //     if (!this->rw(sector + i, 1, reinterpret_cast<uint16_t*>(this->buffer), false)) return false;
    //     memcpy(buffer, this->buffer, 512);
    //     buffer += 512;
    // }
    // return true;

    return rw(sector, sectorCount, reinterpret_cast<uint16_t*>(buffer), false);
}
bool ATADevice::write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    // for (size_t i = 0; i < sectorCount; i++)
    // {
    //     if (!this->rw(sector + i, 1, reinterpret_cast<uint16_t*>(this->buffer), false)) return true;
    //     memcpy(buffer, this->buffer, 512);
    //     buffer += 512;
    // }
    // return true;

    return rw(sector, sectorCount, reinterpret_cast<uint16_t*>(buffer), true);
}

ATADevice::ATADevice(size_t port, size_t drive, ATAController *parent)
{
    this->parent = parent;
    this->port = port;
    this->drive = drive;

    this->buffer = static_cast<uint8_t*>(pmm::alloc());
    this->prdt = static_cast<uint64_t*>(pmm::alloc());

    *this->prdt = ATA_PRD_BUFFER(reinterpret_cast<uint64_t>(this->buffer) | (static_cast<uint64_t>(0x1000) << 32) | ATA_PRD_END);
}

void ATAController::outreg(uint8_t port, uint8_t reg, uint8_t val)
{
    outb((port ? this->port1 : this->port0) + reg, val);
}
uint8_t ATAController::inreg(uint8_t port, uint8_t reg)
{
    return inb((port ? this->port1 : this->port0) + reg);
}

void ATAController::outctrlreg(uint8_t port, uint8_t reg, uint8_t val)
{
    outb((port ? this->ctrlport1 : this->ctrlport0) + reg, val);
}
uint8_t ATAController::inctrlreg(uint8_t port, uint8_t reg)
{
    return inb((port ? this->ctrlport1 : this->ctrlport0) + reg);
}

bool ATAController::detect(size_t port, size_t drive)
{
    this->outreg(port, ATA_REGISTER_DRIVE_HEAD, 0xA0 | (drive << 4));
    for (size_t i = 0; i < 4; i++) inb(this->ctrlport0);

    this->outreg(port, ATA_REGISTER_SECTOR_COUNT, 0);
    this->outreg(port, ATA_REGISTER_LBA_LOW, 0);
    this->outreg(port, ATA_REGISTER_LBA_MID, 0);
    this->outreg(port, ATA_REGISTER_LBA_HIGH, 0);

    this->outreg(port, ATA_REGISTER_COMMAND, ATA_CMD_IDENTIFY);
    timer::msleep(1);

    if (!this->inreg(port, ATA_REGISTER_STATUS)) return false;

    size_t timer = 0xFFFFFF;
    while (timer--)
    {
        uint8_t status = this->inreg(port, ATA_REGISTER_STATUS);
        if (status & ATA_DEV_ERR) return false;
        if (!(status & ATA_DEV_BUSY) && (status & ATA_DEV_DRQ)) break;
    }
    if (timer <= 0) return false;

    if (this->inreg(port, ATA_REGISTER_LBA_MID) || this->inreg(port, ATA_REGISTER_LBA_HIGH)) return false;

    timer = 0xFFFFFF;
    while (timer--)
    {
        uint8_t status = this->inreg(port, ATA_REGISTER_STATUS);
        if (status & ATA_DEV_ERR) return false;
        if (status & ATA_DEV_DRQ) return true;
    }
    return false;
}

ATAController::ATAController(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering ATA driver #%zu", devices.size());

    pci::pcibar bar = this->pcidevice->get_bar(0);
    if (bar.address != 0 && !bar.mmio) this->port0 = bar.address;

    bar = this->pcidevice->get_bar(1);
    if (bar.address != 0 && !bar.mmio) this->port1 = bar.address;

    bar = this->pcidevice->get_bar(2);
    if (bar.address != 0 && !bar.mmio) this->ctrlport0 = bar.address;

    bar = this->pcidevice->get_bar(3);
    if (bar.address != 0 && !bar.mmio) this->ctrlport1 = bar.address;

    bar = this->pcidevice->get_bar(4);
    if (bar.address == 0 || bar.mmio) return;
    this->bmport = bar.address;

    this->pcidevice->command(pci::CMD_BUS_MAST, true);

    for (size_t i = 0; i < 2; i++)
    {
        this->outctrlreg(i, 0x00, this->inctrlreg(i, 0x00) | 0x04);
        for (size_t t = 0; t < 4; t++) inb(ctrlport0);
        this->outctrlreg(i, 0x00, 0x00);

        for (size_t t = 0; t < 2; t++)
        {
            if (this->detect(i, t))
            {
                for (size_t j = 0; j < 256; j++) inw((i ? port1 : port0) + ATA_REGISTER_DATA);
                this->drives.push_back(new ATADevice(i, t, this));
            }
        }
    }

    if (this->drives[0] == nullptr) return;
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
        if (devices.front()->initialised == false)
        {
            free(devices.front());
            devices.pop_back();
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
    error("No ATA devices found!\n");
}
}