# Kernel project
My first os built from scratch\
Contributors are welcome

## [LICENSE](LICENSE)

## TODO

### System
- [x] GDT
- [x] IDT
- [x] TSS
- [x] PCI
- [x] PCIe
- [x] MSI
- [ ] MSI-X

### Memory
- [x] PMM
- [x] VMM
- [x] Heap

### ACPI
- [x] ACPI
- [x] LAPIC
- [x] IOAPIC
- [x] Shutdown
- [x] Reboot

### Device drivers
#### Audio
- [x] PC speaker
- [ ] AC97
- [ ] SB16

#### I/O
- [x] PS/2 Keyboard
- [x] PS/2 Mouse
- [x] COM

#### VMs
- [x] VMWare Tools
- [ ] VBox Guest Additions
- [ ] Virtio

#### Storage
- [ ] FDC
- [x] IDE
- [x] SATA
- [ ] NVMe
- [ ] Virtio block

#### Network
- [x] RTL8139
- [x] RTL8169 (Broken)
- [x] E1000
- [ ] Virtio network

#### USB
- [ ] UHCI
- [ ] OHCI
- [ ] EHCI
- [ ] XHCI

### Timers
- [x] HPET
- [x] PIT
- [x] RTC
- [x] LAPIC Timer

### Tasking
- [x] SMP
- [x] Scheduling
- [ ] IPC

### Partition tables
- [x] MBR
- [x] GPT

### Filesystems
- [x] VFS (Based on Vinix)
- [x] TMPFS
- [x] DEVFS
- [x] Initrd
- [ ] Echfs
- [ ] SFS
- [ ] Ext2
- [ ] Fat32
- [ ] ISO9660
- [ ] NTFS

### Userspace
- [x] System calls
- [x] ELF
- [ ] Signals
- [ ] Userspace
- [ ] Libc
- [ ] Bash
- [ ] Coreutils

### Network stack
- [x] Ethernet
- [x] ARP
- [x] IPv4
- [ ] IPv4 fragmentation support
- [x] ICMPv4
- [ ] TCP
- [ ] UDP
- [ ] DHCP
- [ ] HTTP
- [ ] Telnet
- [ ] SSL

## Building And Running

Make sure you have following programs installed:
* Clang
* lld
* llvm
* Make
* Nasm
* Qemu x86-64
* Xorriso
* Wget
* Tar

If you have Debian based system (Ubuntu, linux mint, Pop_os! etc) you can install them with this command:\
```sudo apt install clang lld make nasm qemu-system-x86 xorriso wget tar```

Follow these steps to build and run the os
1. Clone this repo with:\
``git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/kernel``

2. Go to the root directory of cloned repo and run:\
``make -j$(nproc --all)`` For UEFI mode\
``make bios -j$(nproc --all)`` For BIOS mode

If you, for some reason, are on Termux, run this command instead and connect to ``127.0.0.1:5901`` with VNC viewer:\
``make vnc -j$(nproc --all)``

### Options
* ``USEILAR=1``: Use ILAR archive format for initrd instead of USTAR (requires libboost-iostreams-dev)
* ``NOUBSAN=1``: Turn off UBSAN
* ``NOCLEAN=1``: Don't clean the source after compiling
* ``NORUN=1``: Don't run the kernel, just compile
* ``DEBUG=1``: Turn off kvm and enable qemu logging

## Discord server
https://discord.gg/fM5GK3RpS7

## Resources used:
* Osdev wiki: https://wiki.osdev.org
* Osdev discord server: https://discord.gg/RnCtsqD
* PonchoOS: https://github.com/Absurdponcho/PonchoOS
* Poncho discord server: https://discord.gg/N2Dpwpu4qT
* Limine: https://github.com/limine-bootloader/limine
* Vinix: https://github.com/vlang/vinix
* Lyre: https://github.com/lyre-os/lyre
* Polaris: https://github.com/NSG650/Polaris
* Ilar: https://github.com/ilobilo/ilar
* Printf: https://github.com/eyalroz/printf
* Scalable Screen Font: https://gitlab.com/bztsrc/scalable-font2
* Liballoc: https://github.com/blanham/liballoc
* CWalk: https://github.com/likle/cwalk
* String: https://github.com/cocoz1/cpp-string
* Cxxshim: https://github.com/managarm/cxxshim
* Unifont: https://ftp.gnu.org/gnu/unifont/unifont-14.0.02