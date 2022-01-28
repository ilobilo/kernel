// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/alloc.hpp>

namespace kernel::system::pci {

const char *device_classes[20]
{
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge Device",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satellite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
    "Processing Accelerator",
    "Non Essential Instrumentation"
};

const char *getvendorname(uint16_t vendorid)
{
    switch (vendorid)
    {
        case 0x1234:
            return "QEMU";
        case 0x8086:
            return "Intel";
        case 0x1022:
            return "AMD";
        case 0x10DE:
            return "NVIDIA";
        case 0x10EC:
            return "Realtek";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", vendorid);
    return ret;
}

const char *getdevicename(uint16_t vendorid, uint16_t deviceid)
{
    switch (vendorid)
    {
        case 0x1234:
            switch (deviceid)
            {
                case 0x1111:
                    return "Virtual Video Controller";
            }
            break;
        case 0x8086:
            switch (deviceid)
            {
                case 0x9B63:
                    return "Host Bridge";
                case 0xA3Af:
                    return "USB 3 Controller";
                case 0xA3B1:
                    return "Signal Processing Controller";
                case 0xA3BA:
                    return "Communication Controller";
                case 0xA382:
                    return "SATA Controller";
                case 0xA3DA:
                    return "ISA Bridge";
                case 0xA3A1:
                    return "Memory Controller";
                case 0xa3f0:
                    return "Audio Device";
                case 0xA3A3:
                    return "System Management Bus";
                case 0x1911:
                    return "Xeon E3-1200 v5/v6 / E3-1500 v5 / 6th/7th Gen Core Processor Gaussian Mixture Model";
                case 0x1901:
                    return "Xeon E3-1200 v5/E3-1500 v5/6th Gen Core Processor PCIe Controller (x16)";
                case 0x0D55:
                    return "Ethernet Connection (12) I219-V";
                case 0x29C0:
                    return "82G33/G31/P35/P31 Express DRAM Controller";
                case 0x10D3:
                    return "82574L Gigabit Network Connection";
                case 0x1004:
                    return "82543GC Gigabit Ethernet Controller (Copper)";
                case 0x100E:
                    return "82540EM Gigabit Ethernet Controller";
                case 0x100F:
                    return "82545EM Gigabit Ethernet Controller (Copper)";
                case 0x2934:
                    return "82801I (ICH9 Family) USB UHCI Controller";
                case 0x2935:
                    return "82801I (ICH9 Family) USB UHCI Controller";
                case 0x2936:
                    return "82801I (ICH9 Family) USB UHCI Controller";
                case 0x293A:
                    return "82801I (ICH9 Family) USB2 EHCI Controller";
                case 0x2918:
                    return "82801IB (ICH9) LPC Interface Controller";
                case 0x2922:
                    return "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller";
                case 0x2930:
                    return "82801I (ICH9 Family) SMBus Controller";
            }
            break;
        case 0x10EC:    
            switch (deviceid)
            {
                case 0x8139:
                    return "RTL-8100/8101L/8139 PCI Fast Ethernet Adapter";
            }
            break;
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", deviceid);
    return ret;
}

const char *unclasssubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Non-VGA-Compatible Unclassified Device";
        case 0x01:
            return "VGA-Compatible Unclassified Device";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *mscsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "SCSI Bus Controller";
        case 0x01:
            return "IDE Controller";
        case 0x02:
            return "Floppy Disk Controller";
        case 0x03:
            return "IPI Bus Controller";
        case 0x04:
            return "RAID Controller";
        case 0x05:
            return "ATA Controller";
        case 0x06:
            return "Serial ATA";
        case 0x07:
            return "Serial Attached SCSI";
        case 0x08:
            return "Non-Volatile Memory Controller";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *netsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Ethernet Controller";
        case 0x01:
            return "Token Ring Controller";
        case 0x02:
            return "FDDI Controller";
        case 0x03:
            return "ATM Controller";
        case 0x04:
            return "ISDN Controller";
        case 0x05:
            return "WorldFip Controller";
        case 0x06:
            return "PICMG 2.14 Multi Computing Controller";
        case 0x07:
            return "Infiniband Controller";
        case 0x08:
            return "Fabric Controller";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *dispsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "VGA Compatible Controller";
        case 0x01:
            return "XGA Controller";
        case 0x02:
            return "3D Controller (Not VGA-Compatible)";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *multimediasubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Multimedia Video Controller";
        case 0x01:
            return "Multimedia Audio Controller";
        case 0x02:
            return "Computer Telephony Device";
        case 0x03:
            return "Audio Device";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *memsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "RAM Controller";
        case 0x01:
            return "Flash Controller";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *bridgesubclassname(uint8_t subclasscode){
    switch (subclasscode){
        case 0x00:
            return "Host Bridge";
        case 0x01:
            return "ISA Bridge";
        case 0x02:
            return "EISA Bridge";
        case 0x03:
            return "MCA Bridge";
        case 0x04:
            return "PCI-to-PCI Bridge";
        case 0x05:
            return "PCMCIA Bridge";
        case 0x06:
            return "NuBus Bridge";
        case 0x07:
            return "CardBus Bridge";
        case 0x08:
            return "RACEway Bridge";
        case 0x09:
            return "PCI-to-PCI Bridge";
        case 0x0A:
            return "InfiniBand-to-PCI Host Bridge";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *simplecomsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Serial Controller";
        case 0x01:
            return "Parallel Controller";
        case 0x02:
            return "Multiport Serial Controller";
        case 0x03:
            return "Modem";
        case 0x04:
            return "IEEE 488.1/2 (GPIB) Controller";
        case 0x05:
            return "Smart Card Controller";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *basesyspersubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "PIC";
        case 0x01:
            return "DMA Controller";
        case 0x02:
            return "Timer";
        case 0x03:
            return "RTC Controller";
        case 0x04:
            return "PCI Hot-Plug Controller";
        case 0x05:
            return "SD Host controller";
        case 0x06:
            return "IOMMU";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *inputdevsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Keyboard Controller";
        case 0x01:
            return "Digitizer Pen";
        case 0x02:
            return "Mouse Controller";
        case 0x03:
            return "Scanner Controller";
        case 0x04:
            return "Gameport Controller";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *dockstatsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Generic";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *procsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "386";
        case 0x01:
            return "486";
        case 0x02:
            return "Pentium";
        case 0x03:
            return "Pentium Pro";
        case 0x10:
            return "Alpha";
        case 0x20:
            return "PowerPC";
        case 0x30:
            return "MIPS";
        case 0x40:
            return "Co-Processor";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *sbcsubclassname(uint8_t subclasscode){
    switch (subclasscode){
        case 0x00:
            return "FireWire (IEEE 1394) Controller";
        case 0x01:
            return "ACCESS Bus";
        case 0x02:
            return "SSA";
        case 0x03:
            return "USB Controller";
        case 0x04:
            return "Fibre Channel";
        case 0x05:
            return "SMBus";
        case 0x06:
            return "Infiniband";
        case 0x07:
            return "IPMI Interface";
        case 0x08:
            return "SERCOS Interface (IEC 61491)";
        case 0x09:
            return "CANbus";
        case 0x80:
            return "SerialBusController - Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *wirelsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "iRDA Compatible Controller";
        case 0x01:
            return "Consumer IR Controller";
        case 0x10:
            return "RF Controller";
        case 0x11:
            return "Bluetooth Controller";
        case 0x12:
            return "Broadband Controller";
        case 0x20:
            return "Ethernet Controller (802.1a)";
        case 0x21:
            return "Ethernet Controller (802.1b)";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *intelsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "I20";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *satcomsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Satellite TV Controller";
        case 0x01:
            return "Satellite Audio Controller";
        case 0x02:
            return "Satellite Voice Controller";
        case 0x03:
            return "Satellite Data Controller";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *encryptsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "Network and Computing Encrpytion/Decryption";
        case 0x10:
            return "Entertainment Encryption/Decryption";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *signprocsubclassname(uint8_t subclasscode)
{
    switch (subclasscode)
    {
        case 0x00:
            return "DPIO Modules";
        case 0x01:
            return "Performance Counters";
        case 0x10:
            return "Communication Synchronizer";
        case 0x20:
            return "Signal Processing Management";
        case 0x80:
            return "Other";
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *getsubclassname(uint8_t classcode, uint8_t subclasscode)
{
    switch (classcode)
    {
        case 0x00:
            return unclasssubclassname(subclasscode);
        case 0x01:
            return mscsubclassname(subclasscode);
        case 0x02:
            return netsubclassname(subclasscode);
        case 0x03:
            return dispsubclassname(subclasscode);
        case 0x04:
            return multimediasubclassname(subclasscode);
        case 0x05:
            return memsubclassname(subclasscode);
        case 0x06:
            return bridgesubclassname(subclasscode);
        case 0x07:
            return simplecomsubclassname(subclasscode);
        case 0x08:
            return basesyspersubclassname(subclasscode);
        case 0x09:
            return inputdevsubclassname(subclasscode);
        case 0x0A:
            return dockstatsubclassname(subclasscode);
        case 0x0B:
            return procsubclassname(subclasscode);
        case 0x0C:
            return sbcsubclassname(subclasscode);
        case 0x0D:
            return wirelsubclassname(subclasscode);
        case 0x0E:
            return intelsubclassname(subclasscode);
        case 0x0F:
            return satcomsubclassname(subclasscode);
        case 0x10:
            return encryptsubclassname(subclasscode);
        case 0x11:
            return signprocsubclassname(subclasscode);
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", subclasscode);
    return ret;
}

const char *getprogifname(uint8_t classcode, uint8_t subclasscode, uint8_t progif)
{
    switch (classcode)
    {
        case 0x01:
            switch (subclasscode)
            {
                case 0x01:
                    switch (progif)
                    {
                        case 0x00:
                            return "ISA Compatibility mode-only controller";
                        case 0x05:
                            return "PCI native mode-only controller";
                        case 0x0A:
                            return "ISA Compatibility mode controller, supports both channels switched to PCI native mode";
                        case 0x0F:
                            return "PCI native mode controller, supports both channels switched to ISA compatibility mode";
                        case 0x80:
                            return "ISA Compatibility mode-only controller, supports bus mastering";
                        case 0x85:
                            return "PCI native mode-only controller, supports bus mastering";
                        case 0x8A:
                            return "ISA Compatibility mode controller, supports both channels switched to PCI native mode, supports bus mastering";
                        case 0x8F:
                            return "PCI native mode controller, supports both channels switched to ISA compatibility mode, supports bus mastering";
                    }
                    break;
                case 0x05:
                    switch (progif)
                    {
                        case 0x20:
                            return "Single DMA";
                        case 0x30:
                            return "Chained DMA";
                    }
                    break;
                case 0x06:
                    switch (progif)
                    {
                        case 0x00:
                            return "Vendor Specific Interface";
                        case 0x01:
                            return "AHCI 1.0";
                        case 0x02:
                            return "Serial Storage Bus";
                    }
                    break;
                case 0x07:
                    switch (progif)
                    {
                        case 0x00:
                            return "SAS";
                        case 0x01:
                            return "Serial Storage Bus";
                    }
                    break;
                case 0x08:
                    switch (progif)
                    {
                        case 0x01:
                            return "NVMHCI";
                        case 0x02:
                            return "NVM Express";
                    }
                    break;
            }
            break;
        case 0x03:
            switch (subclasscode)
            {
                case 0x00:
                    switch (progif)
                    {
                        case 0x00:
                            return "VGA Controller";
                        case 0x01:
                            return "8514-Compatible Controller";
                    }
                    break;
            }
            break;
        case 0x06:
            switch (subclasscode)
            {
                case 0x04:
                    switch (progif)
                    {
                        case 0x00:
                            return "Normal Decode";
                        case 0x01:
                            return "Subtractive Decode";
                    }
                    break;
                case 0x08:
                    switch (progif)
                    {
                        case 0x00:
                            return "Transparent Mode";
                        case 0x01:
                            return "Endpoint Mode";
                    }
                    break;
                case 0x09:
                    switch (progif)
                    {
                        case 0x40:
                            return "Semi-Transparent, Primary bus towards host CPU";
                        case 0x80:
                            return "Semi-Transparent, Secondary bus towards host CPU";
                    }
                    break;
            }
            break;
        case 0x07:
            switch (subclasscode)
            {
                case 0x00:
                    switch (progif)
                    {
                        case 0x00:
                            return "8250-Compatible (Generic XT)";
                        case 0x01:
                            return "16450-Compatible";
                        case 0x02:
                            return "16550-Compatible";
                        case 0x03:
                            return "16650-Compatible";
                        case 0x04:
                            return "16750-Compatible";
                        case 0x05:
                            return "16850-Compatible";
                        case 0x06:
                            return "16950-Compatible";
                    }
                    break;
                case 0x01:
                    switch (progif)
                    {
                        case 0x00:
                            return "Standard Parallel Port";
                        case 0x01:
                            return "Bi-Directional Parallel Port";
                        case 0x02:
                            return "ECP 1.X Compliant Parallel Port";
                        case 0x03:
                            return "IEEE 1284 Controller";
                        case 0xFE:
                            return "IEEE 1284 Target Device";
                    }
                    break;
                case 0x03:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic Modem";
                        case 0x01:
                            return "Hayes 16450-Compatible Interface";
                        case 0x02:
                            return "Hayes 16550-Compatible Interface";
                        case 0x03:
                            return "Hayes 16650-Compatible Interface";
                        case 0x04:
                            return "Hayes 16750-Compatible Interface";
                    }
                    break;
            }
            break;
        case 0x08:
            switch (subclasscode)
            {
                case 0x00:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic 8259-Compatible";
                        case 0x01:
                            return "ISA-Compatible";
                        case 0x02:
                            return "EISA-Compatible";
                        case 0x10:
                            return "I/O APIC Interrupt Controller";
                        case 0x20:
                            return "I/O(x) APIC Interrupt Controller";
                    }
                    break;
                case 0x01:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic 8237-Compatible";
                        case 0x01:
                            return "ISA-Compatible";
                        case 0x02:
                            return "EISA-Compatible";
                    }
                    break;
                case 0x02:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic 8254-Compatible";
                        case 0x01:
                            return "ISA-Compatible";
                        case 0x02:
                            return "EISA-Compatible";
                        case 0x03:
                            return "HPET";
                    }
                    break;
                case 0x03:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic RTC";
                        case 0x01:
                            return "ISA-Compatible";
                    }
                    break;
            }
            break;
        case 0x09:
            switch (subclasscode)
            {
                case 0x04:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic";
                        case 0x10:
                            return "Extended";
                    }
                    break;
            }
            break;
        case 0x0C:
            switch (subclasscode)
            {
                case 0x00:
                    switch (progif)
                    {
                        case 0x00:
                            return "Generic";
                        case 0x10:
                            return "OHCI";
                    }
                    break;
                case 0x03:
                    switch (progif)
                    {
                        case 0x00:
                            return "UHCI Controller";
                        case 0x10:
                            return "OHCI Controller";
                        case 0x20:
                            return "EHCI (USB2) Controller";
                        case 0x30:
                            return "XHCI (USB3) Controller";
                        case 0x80:
                            return "Unspecified";
                        case 0xFE:
                            return "USB Device (Not a Host Controller)";
                    }
                    break;
                case 0x07:
                    switch (progif)
                    {
                        case 0x00:
                            return "SMIC";
                        case 0x01:
                            return "Keyboard Controller Style";
                        case 0x02:
                            return "Block Transfer";
                    }
                    break;
            }
            break;
    }
    char *ret = static_cast<char*>(calloc(6, sizeof(char)));
    sprintf(ret, "%.4X", progif);
    return ret;
}
}