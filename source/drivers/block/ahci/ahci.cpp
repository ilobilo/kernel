// Copyright (C) 2021-2022  ilobilo

#include <drivers/block/ahci/ahci.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <lib/shared_ptr.hpp>
#include <lib/memory.hpp>
#include <lib/timer.hpp>
#include <lib/log.hpp>

using namespace kernel::system::mm;

namespace kernel::drivers::block::ahci {

bool initialised = false;
vector<AHCIController*> devices;

void AHCIPort::stopCMD()
{
    this->hbaport->CommandStatus &= ~HBA_PxCMD_ST;
    this->hbaport->CommandStatus &= ~HBA_PxCMD_FRE;

    while (true)
    {
        if (this->hbaport->CommandStatus & HBA_PxCMD_FR) continue;
        if (this->hbaport->CommandStatus & HBA_PxCMD_CR) continue;
        break;
    }
}

void AHCIPort::startCMD()
{
    this->hbaport->CommandStatus &= ~HBA_PxCMD_ST;
    while (this->hbaport->CommandStatus & HBA_PxCMD_CR);
    this->hbaport->CommandStatus |= HBA_PxCMD_FRE;
    this->hbaport->CommandStatus |= HBA_PxCMD_ST;
}

size_t AHCIPort::findSlot()
{
    uint32_t slots = this->hbaport->SataActive | this->hbaport->CommandIssue;
    for (uint8_t i = 0; i < 32; i++)
    {
        if ((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

bool AHCIPort::rw(uint64_t sector, uint32_t sectorCount, uint8_t *buffer, bool write)
{
    if (this->portType == AHCIPortType::SATAPI && write)
    {
        error("AHCI: Port #%d: Can not write to ATAPI drive!", this->portNum);
        return false;
    }

    lockit(this->lock);

    this->hbaport->InterruptEnable = 0xFFFFFFFF;
    this->hbaport->InterruptStatus = 0;

    int slot = findSlot();
    if (slot == -1) return false;

    this->hbaport->TaskFileData = this->hbaport->SataError = 0;

    HBACommandHeader *cmdHdr = reinterpret_cast<HBACommandHeader*>(this->hbaport->CommandListBase | static_cast<uint64_t>(this->hbaport->CommandListBaseUpper) << 32);
    cmdHdr += slot;
    cmdHdr->CommandFISLength = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    // cmdHdr->ATAPI = (this->portType == SATAPI ? 1 : 0);
    cmdHdr->ATAPI = 0;
    cmdHdr->Write = write;
    cmdHdr->ClearBusy = 0;
    cmdHdr->Prefetchable = 0;

    cmdHdr->PRDBCount = 0;
    cmdHdr->PortMultiplier = 0;
    cmdHdr->PRDTLength = static_cast<uint16_t>((sectorCount - 1) >> 4) + 1;

    HBACommandTable *cmdtable = reinterpret_cast<HBACommandTable*>(cmdHdr->CommandTableBaseAddress | static_cast<uint64_t>(cmdHdr->CommandTableBaseAddressUpper) << 32);
    memset(cmdtable, 0, sizeof(HBACommandTable) + (cmdHdr->PRDTLength - 1) * sizeof(HBAPRDTEntry));

    size_t i = 0;
    for (; i < cmdHdr->PRDTLength - 1; i++)
    {
        cmdtable->PRDTEntry[i].DataBaseAddress = static_cast<uint32_t>(reinterpret_cast<uint64_t>(buffer));
        cmdtable->PRDTEntry[i].DataBaseAddressUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(buffer) << 32);
        cmdtable->PRDTEntry[i].ByteCount = 0x2000 - 1;
        cmdtable->PRDTEntry[i].InterruptOnCompletion = 1;
        buffer += 0x1000;
        sectorCount -= 16;
    }

    cmdtable->PRDTEntry[i].DataBaseAddress = static_cast<uint32_t>(reinterpret_cast<uint64_t>(buffer));
    cmdtable->PRDTEntry[i].DataBaseAddressUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(buffer) << 32);
    cmdtable->PRDTEntry[i].ByteCount = (sectorCount << 9) - 1;
    cmdtable->PRDTEntry[i].InterruptOnCompletion = 1;
    // if (this->portType == SATAPI)
    // {
    //     cmdtable->ATAPICommand[0] = ATAPI_CMD_READ;
    //     cmdtable->ATAPICommand[2] = static_cast<uint8_t>(sector >> 24);
    //     cmdtable->ATAPICommand[3] = static_cast<uint8_t>(sector >> 16);
    //     cmdtable->ATAPICommand[4] = static_cast<uint8_t>(sector >> 8);
    //     cmdtable->ATAPICommand[5] = static_cast<uint8_t>(sector);
    //     cmdtable->ATAPICommand[9] = sectorCount;
    // }

    FIS_REG_H2D *cmdFIS = reinterpret_cast<FIS_REG_H2D*>(&cmdtable->CommandFIS);
    cmdFIS->FISType = FIS_TYPE::FIS_TYPE_REG_H2D;
    cmdFIS->CommandControl = 1;
    cmdFIS->PortMultiplier = 0;
    // cmdFIS->Command = (this->portType == SATAPI ? ATAPI_CMD_PACKET : (write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX));
    cmdFIS->Command = (write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX);

    cmdFIS->LBA0 = static_cast<uint8_t>(sector);
    cmdFIS->LBA1 = static_cast<uint8_t>(sector >> 8);
    cmdFIS->LBA2 = static_cast<uint8_t>(sector >> 16);
    cmdFIS->LBA3 = static_cast<uint8_t>(sector >> 24);
    cmdFIS->LBA4 = static_cast<uint8_t>(sector >> 32);
    cmdFIS->LBA5 = static_cast<uint8_t>(sector >> 40);

    cmdFIS->DeviceRegister = 1 << 6;

    cmdFIS->CountLow = static_cast<uint8_t>(sectorCount);
    cmdFIS->CountHigh = static_cast<uint8_t>(sectorCount >> 8);

    size_t spin = 100;
    while (spin-- && (this->hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))) timer::msleep(1);
    if (spin <= 0)
    {
        error("AHCI: Port #%d: Hung!", this->portNum);
        return false;
    }

    this->hbaport->InterruptEnable = this->hbaport->InterruptStatus = 0xFFFFFFFF;

    this->startCMD();
    this->hbaport->CommandIssue |= 1 << slot;

    while (this->hbaport->CommandIssue & (1 << slot))
    {
        if (this->hbaport->InterruptStatus & HBA_PxIS_TFES)
        {
            error("AHCI: Port #%d: %s error!", this->portNum, write ? "Write" : "Read");
            this->stopCMD();
            return false;
        }
    }

    spin = 100;
    while (spin-- && (this->hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))) timer::msleep(1);

    this->stopCMD();
    if (spin <= 0)
    {
        error("AHCI: Port #%d: Hung!", this->portNum);
        return false;
    }

    if (this->hbaport->InterruptStatus & HBA_PxIS_TFES)
    {
        error("AHCI: Port #%d: %s error!", this->portNum, write ? "Write" : "Read");
        return false;
    }

    return true;
}

bool AHCIPort::identify()
{
    lockit(this->lock);
    std::shared_ptr<uint16_t> data(new uint16_t[256]);

    this->hbaport->InterruptEnable = 0xFFFFFFFF;
    this->hbaport->InterruptStatus = 0;

    int slot = findSlot();
    if (slot == -1) return false;

    HBACommandHeader *cmdHdr = reinterpret_cast<HBACommandHeader*>(this->hbaport->CommandListBase | static_cast<uint64_t>(this->hbaport->CommandListBaseUpper) << 32);
    cmdHdr += slot;
    cmdHdr->CommandFISLength = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    // cmdHdr->ATAPI = (this->portType == SATAPI ? 1 : 0);
    cmdHdr->ATAPI = 0;
    cmdHdr->Write = 0;
    cmdHdr->ClearBusy = 0;
    cmdHdr->Prefetchable = 0;

    cmdHdr->PRDBCount = 0;
    cmdHdr->PortMultiplier = 0;
    cmdHdr->PRDTLength = 1;

    HBACommandTable *cmdtable = reinterpret_cast<HBACommandTable*>(cmdHdr->CommandTableBaseAddress | static_cast<uint64_t>(cmdHdr->CommandTableBaseAddressUpper) << 32);
    memset(cmdtable, 0, sizeof(HBACommandTable) + (cmdHdr->PRDTLength - 1) * sizeof(HBAPRDTEntry));

    cmdtable->PRDTEntry[0].DataBaseAddress = static_cast<uint32_t>(reinterpret_cast<uint64_t>(data.get()));
    cmdtable->PRDTEntry[0].DataBaseAddressUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(data.get()) << 32);
    cmdtable->PRDTEntry[0].ByteCount = (1 << 9) - 1;
    cmdtable->PRDTEntry[0].InterruptOnCompletion = 1;

    // if (this->portType == SATAPI)
    // {
    //     cmdtable->ATAPICommand[0] = ATAPI_CMD_CAPACITY;
    //     cmdtable->ATAPICommand[9] = 1;
    // }

    FIS_REG_H2D *cmdFIS = reinterpret_cast<FIS_REG_H2D*>(&cmdtable->CommandFIS);
    cmdFIS->FISType = FIS_TYPE_REG_H2D;
    cmdFIS->CommandControl = 1;
    cmdFIS->PortMultiplier = 0;
    // cmdFIS->Command = (this->portType == AHCIPortType::SATAPI ? ATAPI_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY);
    cmdFIS->Command = ATA_CMD_IDENTIFY;

    cmdFIS->LBA0 = 0;
    cmdFIS->LBA1 = 0;
    cmdFIS->LBA2 = 0;
    cmdFIS->LBA3 = 0;
    cmdFIS->LBA4 = 0;
    cmdFIS->LBA5 = 0;
    cmdFIS->DeviceRegister = 0;

    cmdFIS->CountLow = 0;
    cmdFIS->CountHigh = 0;
    cmdFIS->Control = 0;

    size_t spin = 100;
    while (spin-- && (this->hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))) timer::msleep(1);
    if (spin <= 0)
    {
        error("AHCI: Port #%d: Hung!", this->portNum);
        return false;
    }

    this->hbaport->InterruptEnable = this->hbaport->InterruptStatus = 0xFFFFFFFF;

    this->startCMD();
    this->hbaport->SataControl |= 1 << slot;
    this->hbaport->CommandIssue |= 1 << slot;

    while (this->hbaport->CommandIssue & (1 << slot))
    {
        if (this->hbaport->InterruptStatus & HBA_PxIS_TFES)
        {
            this->stopCMD();
            return false;
        }
    }
    this->stopCMD();

    spin = 100;
    while (spin-- && (this->hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))) timer::msleep(1);
    if (this->hbaport->InterruptStatus & HBA_PxIS_TFES) return false;

    this->sectors = *reinterpret_cast<uint64_t*>(&data[100]);

    return true;
}

AHCIPort::AHCIPort(HBAPort *hbaport, size_t portNum)
{
    this->hbaport = hbaport;
    this->portNum = portNum;
    this->buffer = pmm::alloc<uint8_t*>();

    stopCMD();

    void *newbase = malloc(1024);
    this->hbaport->CommandListBase = static_cast<uint32_t>(reinterpret_cast<uint64_t>(newbase));
    this->hbaport->CommandListBaseUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(newbase) >> 32);

    void *fisBase = malloc(256);
    this->hbaport->FISBaseAddress = static_cast<uint32_t>(reinterpret_cast<uint64_t>(fisBase));
    this->hbaport->FISBaseAddressUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(fisBase) >> 32);

    HBACommandHeader *commandHdr = reinterpret_cast<HBACommandHeader*>(this->hbaport->CommandListBase + (static_cast<uint64_t>(this->hbaport->CommandListBaseUpper) << 32));
    for (size_t i = 0; i < 32; i++)
    {
        commandHdr[i].PRDTLength = 8;
        void *cmdTableAddr = malloc(256);
        uint64_t address = reinterpret_cast<uint64_t>(cmdTableAddr) + (i << 8);
        commandHdr[i].CommandTableBaseAddress = static_cast<uint32_t>(address);
        commandHdr[i].CommandTableBaseAddressUpper = static_cast<uint32_t>(static_cast<uint64_t>(address) >> 32);
    }

    startCMD();

    this->hbaport->InterruptStatus = 0;
    this->hbaport->InterruptEnable = 1;
    this->hbaport->FISSwitchControl &= ~0xFFFFF000U;

    timer::msleep(10);
    int spin = 100;
    while (spin-- && (this->hbaport->SataStatus & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT) timer::msleep(1);
    if ((this->hbaport->SataStatus & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT)
    {
        error("AHCI: Port #%d: Device not present!", this->portNum);
        return;
    }

    this->hbaport->CommandStatus = (this->hbaport->CommandStatus & ~HBA_PxCMD_ICC) | HBA_PxCMD_ICC_ACTIVE;

    spin = 1000;
    while (spin-- && (this->hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))) timer::msleep(1);
    if (spin <= 0)
    {
        warn("AHCI: Port #%d: Hung!", this->portNum);
        this->hbaport->SataControl = SCTL_PORT_DET_INIT | SCTL_PORT_IPM_NOPART | SCTL_PORT_IPM_NOSLUM | SCTL_PORT_IPM_NODSLP;
    }

    timer::msleep(10);
    this->hbaport->SataControl &= ~HBA_PxSSTS_DET;
    timer::msleep(10);

    spin = 200;
    while (spin-- && (this->hbaport->SataStatus & HBA_PxSSTS_DET_PRESENT) != HBA_PxSSTS_DET_PRESENT) timer::msleep(1);

    if ((this->hbaport->TaskFileData & 0xFF) == 0xFF) timer::msleep(500);

    this->hbaport->SataError = 0;
    this->hbaport->InterruptStatus = 0;

    spin = 1000;
    while (spin-- && (this->hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))) timer::msleep(1);

    if (spin <= 0) warn("AHCI: Port #%d: Hung!", this->portNum);

    if ((this->hbaport->SataStatus & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT)
    {
        error("AHCI: Port #%d: Device not present!", this->portNum);
        return;
    }

    if (!this->identify())
    {
        error("AHCI: Port #%d: Identify error!", this->portNum);
        return;
    }

    this->stat.blocks = this->sectors;

    this->stat.blksize = 512;
    this->stat.size = this->stat.blocks * this->stat.blksize;
    this->stat.rdev = vfs::dev_new_id();
    this->stat.mode = 0644 | vfs::stats::ifblk;

    this->initialised = true;
}

AHCIController::AHCIController(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering AHCI driver #%zu", devices.size());

    pcidevice->command(pci::CMD_BUS_MAST | pci::CMD_MEM_SPACE, true);

    this->ABAR = reinterpret_cast<HBAMemory*>(pcidevice->get_bar(5).address);

    if (this->ABAR->Version >= 0x10200 && (this->ABAR->HostCapabilitiesExtended & 1))
    {
        this->ABAR->BIOSHandoffControlStatus |= AHCI_OWNER_OS;

        size_t spin = 25;
        while (spin-- && (this->ABAR->BIOSHandoffControlStatus & AHCI_OWNER_BIOS)) timer::msleep(1);

        if (spin <= 0)
        {
            if (this->ABAR->BIOSHandoffControlStatus & AHCI_BIOS_BUSY)
            {
                warn("AHCI: BIOS handoff timed out! Retrying...");
                spin = 25;
                while (spin-- && (this->ABAR->BIOSHandoffControlStatus & AHCI_OWNER_BIOS)) timer::msleep(1);
                if (spin <= 0)
                {
                    error("AHCI: BIOS handoff timed out!");
                    return;
                }
            }
            else warn("AHCI: BIOS handoff timed out!");
        }
    }

    this->ABAR->GlobalHostControl = this->ABAR->GlobalHostControl | AHCI_GHC_HR;
    size_t spin = 100;
    while (spin-- && (this->ABAR->GlobalHostControl & AHCI_GHC_HR)) timer::msleep(1);
    if (spin <= 0)
    {
        error("AHCI: HBA timed out!");
        return;
    }
    this->ABAR->GlobalHostControl |= AHCI_GHC_AE;

    uint32_t cap = this->ABAR->HostCapability;
    uint8_t maxports = (cap & 0xF) + 1;
    if (maxports > 32)
    {
        error("AHCI: Maximum port count is higher than 32!");
        return;
    }

    uint32_t portsImpl = ABAR->PortsImplemented;
    if (portsImpl == 0)
    {
        error("AHCI: No implemented ports!");
        return;
    }

    while (!(this->ABAR->GlobalHostControl & AHCI_GHC_IE))
    {
        this->ABAR->GlobalHostControl |= AHCI_GHC_IE;
        timer::msleep(1);
    }
    this->ABAR->GlobalHostControl |= AHCI_GHC_IE;
    this->ABAR->InterruptStatus = 0xFFFFFFFF;

    for (size_t i = 0; i < maxports; i++)
    {
        if (portsImpl & (1 << i))
        {
            auto port = &this->ABAR->Ports[i];
            uint32_t statStat = port->SataStatus;
            uint8_t intpowman = (statStat >> 8) & 0b111;
            uint8_t devdetect = statStat & 0b111;

            if (devdetect != HBA_PORT_DEV_PRESENT) continue;
            if (intpowman != HBA_PORT_IPM_ACTIVE) continue;

            this->ports.push_back(new AHCIPort(port, ports.size()));
            if (this->ports.back()->initialised == false)
            {
                free(this->ports.back());
                this->ports.pop_back();
                error("Could not initialise AHCI port #%zu", this->ports.size());
            }
        }
    }

    if (ports.size() == 0)
    {
        error("AHCI: No ports found!");
        return;
    }

    this->initialised = true;
}

void init()
{
    log("Initialising AHCI driver");

    if (initialised)
    {
        warn("AHCI driver has already been initialised!\n");
        return;
    }

    size_t count = pci::count(0x01, 0x06, 0x01);
    if (count == 0)
    {
        error("No AHCI devices found!\n");
        return;
    }

    devices.init(count);
    for (size_t i = 0; i < count; i++)
    {
        devices.push_back(new AHCIController(pci::search(0x01, 0x06, 0x01, i)));
        if (devices.front()->initialised == false)
        {
            free(devices.front());
            devices.pop_back();
        }
    }

    serial::newline();
    if (devices.size() != 0) initialised = true;
}
}