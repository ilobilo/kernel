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

void AHCIController::probePorts()
{
    int portsImpl = ABAR->PortsImplemented;
    for (uint64_t i = 0; i < 32; i++)
    {
        if (portsImpl & (1 << i))
        {
            AHCIPortType portType = checkPortType(&ABAR->Ports[i]);
            if (portType == AHCIPortType::SATA || portType == AHCIPortType::SATAPI)
            {
                ports[portCount] = new AHCIDevice;
                ports[portCount]->portType = portType;
                ports[portCount]->hbaport = &ABAR->Ports[i];
                ports[portCount]->portNum = portCount;
                ports[portCount]->buffer = static_cast<uint8_t*>(pmm::alloc());

                portCount++;
            }
        }
    }
}

void AHCIDevice::configure()
{
    stopCMD();

    void *newbase = malloc(1024);
    hbaport->CommandListBase = static_cast<uint32_t>(reinterpret_cast<uint64_t>(newbase));
    hbaport->CommandListBaseUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(newbase) >> 32);

    void *fisBase = malloc(256);
    hbaport->FISBaseAddress = static_cast<uint32_t>(reinterpret_cast<uint64_t>(fisBase));
    hbaport->FISBaseAddressUpper = static_cast<uint32_t>(reinterpret_cast<uint64_t>(fisBase) >> 32);

    HBACommandHeader *commandHdr = reinterpret_cast<HBACommandHeader*>(hbaport->CommandListBase + (static_cast<uint64_t>(hbaport->CommandListBaseUpper) << 32));
    for (int i = 0; i < 32; i++)
    {
        commandHdr[i].PRDTLength = 8;
        void *cmdTableAddr = malloc(256);
        uint64_t address = reinterpret_cast<uint64_t>(cmdTableAddr) + (i << 8);
        commandHdr[i].CommandTableBaseAddress = static_cast<uint32_t>(address);
        commandHdr[i].CommandTableBaseAddressUpper = static_cast<uint32_t>(static_cast<uint64_t>(address) >> 32);
    }

    startCMD();
}

void AHCIDevice::stopCMD()
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

void AHCIDevice::startCMD()
{
    while (hbaport->CommandStatus & HBA_PxCMD_CR);
    hbaport->CommandStatus |= HBA_PxCMD_FRE;
    hbaport->CommandStatus |= HBA_PxCMD_ST;
}

size_t AHCIDevice::findSlot()
{
    uint32_t slots = hbaport->SataActive | hbaport->CommandIssue;
    for (uint8_t i = 0; i < 32; i++)
    {
        if ((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

[[clang::optnone]] bool AHCIDevice::rw(uint64_t sector, uint32_t sectorCount, uint16_t *buffer, bool write)
{
    if (this->portType == AHCIPortType::SATAPI && write)
    {
        error("Can not write to ATAPI drive!");
        return false;
    }

    this->lock.lock();
    hbaport->InterruptStatus = static_cast<uint32_t>(-1);
    size_t spin = 0;
    int slot = findSlot();
    if (slot == -1) return false;

    HBACommandHeader *cmdHdr = reinterpret_cast<HBACommandHeader*>(hbaport->CommandListBase | static_cast<uint64_t>(hbaport->CommandListBaseUpper) << 32);
    cmdHdr += slot;
    cmdHdr->CommandFISLength = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdHdr->Write = (write) ? 1 : 0;
    cmdHdr->ClearBusy = 1;
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

    FIS_REG_H2D *cmdFIS = reinterpret_cast<FIS_REG_H2D*>(&cmdtable->CommandFIS);
    cmdFIS->FISType = FIS_TYPE::FIS_TYPE_REG_H2D;
    cmdFIS->CommandControl = 1;
    cmdFIS->Command = (write) ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX;

    cmdFIS->LBA0 = static_cast<uint8_t>(sector);
    cmdFIS->LBA1 = static_cast<uint8_t>(sector >> 8);
    cmdFIS->LBA2 = static_cast<uint8_t>(sector >> 16);
    cmdFIS->LBA3 = static_cast<uint8_t>(sector >> 32);
    cmdFIS->LBA4 = static_cast<uint8_t>(sector >> 40);
    cmdFIS->LBA5 = static_cast<uint8_t>(sector >> 48);

    cmdFIS->DeviceRegister = 1 << 6;

    cmdFIS->CountLow = sectorCount & 0xFF;
    cmdFIS->CountHigh = (sectorCount >> 8) & 0xFF;

    while ((hbaport->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) spin++;
    if (spin == 1000000)
    {
        error("AHCI: Port is hung!");
        this->lock.unlock();
        return false;
    }

    hbaport->CommandIssue = 1 << slot;

    while (true)
    {
        if ((hbaport->CommandIssue & (1 << slot)) == 0) break;
        if (hbaport->InterruptStatus & HBA_PxIS_TFES)
        {
            error("AHCI: %s error!", (write) ? "write" : "read");
            this->lock.unlock();
            return false;
        }
    }

    if (hbaport->InterruptStatus & HBA_PxIS_TFES)
    {
        error("AHCI: %s error!", (write) ? "write" : "read");
        this->lock.unlock();
        return false;
    }
    while (hbaport->CommandIssue);

    this->lock.unlock();
    return true;
}

bool AHCIDevice::read(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    return rw(sector, sectorCount, reinterpret_cast<uint16_t*>(buffer), false);
}
bool AHCIDevice::write(uint64_t sector, uint32_t sectorCount, uint8_t *buffer)
{
    return rw(sector, sectorCount, reinterpret_cast<uint16_t*>(buffer), true);
}

AHCIController::AHCIController(pci::pcidevice_t *pcidevice)
{
    this->pcidevice = pcidevice;
    log("Registering AHCI driver #%zu", devices.size());

    ABAR = reinterpret_cast<HBAMemory*>(pcidevice->get_bar(5).address);

    probePorts();

    if (portCount == 0)
    {
        error("AHCI: No ports found!");
        return;
    }

    for (size_t i = 0; i < portCount; i++)
    {
        AHCIDevice *port = ports[i];
        port->configure();

        // MBR bootsector
        // uint8_t mbr[] = { [0 ... 509] = 0, 0x55, 0xAA };
        // memcpy(ports[i]->buffer, mbr, 512);
        // ports[i]->write(0, 2, ports[i]->buffer);
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