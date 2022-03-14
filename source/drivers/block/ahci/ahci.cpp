// Copyright (C) 2021-2022  ilobilo

#include <drivers/block/ahci/ahci.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/log.hpp>

using namespace kernel::system::mm;

namespace kernel::drivers::block::ahci {

bool initialised = false;
vector<AHCIController*> devices;

AHCIPortType checkPortType(HBAPort *port)
{
    uint32_t statStat = port->SataStatus;
    uint8_t intpowman = (statStat >> 8) & 0b111;
    uint8_t devdetect = statStat & 0b111;

    if (devdetect != HBA_PORT_DEV_PRESENT) return AHCIPortType::NONE;
    if (intpowman != HBA_PORT_IPM_ACTIVE) return AHCIPortType::NONE;

    switch (port->Signature)
    {
        case SATA_SIG_ATA:
            return AHCIPortType::SATA;
        case SATA_SIG_ATAPI:
            return AHCIPortType::SATAPI;
        case SATA_SIG_PM:
            return AHCIPortType::PM;
        case SATA_SIG_SEMB:
            return AHCIPortType::SEMB;
        default:
            return AHCIPortType::NONE;
    }
}

void AHCIPort::stopCMD()
{
    hbaport->CommandStatus &= ~HBA_PxCMD_ST;
    hbaport->CommandStatus &= ~HBA_PxCMD_FRE;

    while (true)
    {
        if (hbaport->CommandStatus & HBA_PxCMD_FR) continue;
        if (hbaport->CommandStatus & HBA_PxCMD_CR) continue;
        break;
    }
}

void AHCIPort::startCMD()
{
    while (hbaport->CommandStatus & HBA_PxCMD_CR);
    hbaport->CommandStatus |= HBA_PxCMD_FRE;
    hbaport->CommandStatus |= HBA_PxCMD_ST;
}

size_t AHCIPort::findSlot()
{
    uint32_t slots = hbaport->SataActive | hbaport->CommandIssue;
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
        error("Can not write to ATAPI drive!");
        return false;
    }

    lockit(this->lock);

    hbaport->InterruptStatus = static_cast<uint32_t>(-1);
    size_t spin = 0;
    int slot = findSlot();
    if (slot == -1) return false;

    HBACommandHeader *cmdHdr = reinterpret_cast<HBACommandHeader*>(hbaport->CommandListBase | static_cast<uint64_t>(hbaport->CommandListBaseUpper) << 32);
    cmdHdr += slot;
    cmdHdr->CommandFISLength = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdHdr->Write = write ? 1 : 0;
    cmdHdr->ClearBusy = 1;
    cmdHdr->PRDTLength = static_cast<uint16_t>((sectorCount - 1) >> 4) + 1;
    // if (portType == SATAPI) cmdHdr->ATAPI = 1;

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
    // if (portType == SATAPI)
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
    // cmdFIS->Command = (portType == SATAPI ? ATAPI_PACKET : (write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX));
    cmdFIS->Command = write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX;

    cmdFIS->LBA0 = static_cast<uint8_t>(sector);
    cmdFIS->LBA1 = static_cast<uint8_t>(sector >> 8);
    cmdFIS->LBA2 = static_cast<uint8_t>(sector >> 16);
    cmdFIS->LBA3 = static_cast<uint8_t>(sector >> 24);
    cmdFIS->LBA4 = static_cast<uint8_t>(sector >> 32);
    cmdFIS->LBA5 = static_cast<uint8_t>(sector >> 40);

    cmdFIS->DeviceRegister = 1 << 6;

    cmdFIS->CountLow = sectorCount & 0xFF;
    cmdFIS->CountHigh = (sectorCount >> 8) & 0xFF;

    while ((hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) spin++;
    if (spin == 1000000)
    {
        error("AHCI: Port #%d is hung!", this->portNum);
        return false;
    }

    hbaport->CommandIssue = 1 << slot;

    while (true)
    {
        if ((hbaport->CommandIssue & (1 << slot)) == 0) break;
        if (hbaport->InterruptStatus & HBA_PxIS_TFES)
        {
            error("AHCI: Port #%d %s error!", this->portNum, write ? "write" : "read");
            return false;
        }
    }

    if (hbaport->InterruptStatus & HBA_PxIS_TFES)
    {
        error("AHCI: Port #%d %s error!", this->portNum, write ? "write" : "read");
        return false;
    }
    while (hbaport->CommandIssue);

    return true;
}

bool AHCIPort::read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    return rw(sector, sectorCount, buffer, false);
}
bool AHCIPort::write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    return rw(sector, sectorCount, buffer, true);
}

AHCIPort::AHCIPort(AHCIPortType portType, HBAPort *hbaport, size_t portNum)
{
    this->portType = portType;
    this->hbaport = hbaport;
    this->portNum = portNum;
    this->buffer = static_cast<uint8_t*>(pmm::alloc());

    stopCMD();

    void *newbase = malloc(1024);
    hbaport->CommandListBase = static_cast<uint32_t>(reinterpret_cast<uint64_t>(newbase));
    hbaport->CommandListBaseUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(newbase) >> 32);

    void *fisBase = malloc(256);
    hbaport->FISBaseAddress = static_cast<uint32_t>(reinterpret_cast<uint64_t>(fisBase));
    hbaport->FISBaseAddressUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(fisBase) >> 32);

    HBACommandHeader *commandHdr = reinterpret_cast<HBACommandHeader*>(hbaport->CommandListBase + (static_cast<uint64_t>(hbaport->CommandListBaseUpper) << 32));
    for (size_t i = 0; i < 32; i++)
    {
        commandHdr[i].PRDTLength = 8;
        void *cmdTableAddr = malloc(256);
        uint64_t address = reinterpret_cast<uint64_t>(cmdTableAddr) + (i << 8);
        commandHdr[i].CommandTableBaseAddress = static_cast<uint32_t>(address);
        commandHdr[i].CommandTableBaseAddressUpper = static_cast<uint32_t>(static_cast<uint64_t>(address) >> 32);
    }

    startCMD();
}

AHCIController::AHCIController(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering AHCI driver #%zu", devices.size());

    ABAR = reinterpret_cast<HBAMemory*>(pcidevice->get_bar(5).address);

    uint32_t portsImpl = ABAR->PortsImplemented;
    for (size_t i = 0; i < 32; i++)
    {
        if (portsImpl & (1 << i))
        {
            AHCIPortType portType = checkPortType(&ABAR->Ports[i]);
            if (portType == AHCIPortType::SATA || portType == AHCIPortType::SATAPI)
            {
                ports.push_back(new AHCIPort(portType, &ABAR->Ports[i], ports.size()));
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