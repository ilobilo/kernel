# Kernel project
My first os built from scratch<br />
Contributors are welcome

## [LICENSE](LICENSE)

## TODO

### System
- [x] GDT
- [x] IDT
- [x] TSS
- [x] PCI

### Display
- [x] Drawing on framebuffer
- [x] Serial debugging

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

### Audio
- [x] PC speaker
- [ ] AC97
- [ ] SB16

### Peripherals
- [x] PS/2 Keyboard
- [x] PS/2 Mouse
- [ ] USB Keyboard
- [ ] USB Mouse

### VMs
- [x] VMWare Tools
- [ ] VBox Guest Additions

### Disk
- [ ] FDC
- [ ] IDE (ATA/ATAPI)
- [ ] SATA (AHCI)
- [ ] NVMe

### Timers
- [x] HPET
- [x] PIT
- [x] RTC
- [ ] LAPIC Timer

### Tasking
- [x] SMP
- [x] Threading
- [x] Scheduling

### Filesystem
- [x] VFS
- [x] DEVFS
- [x] Initrd
- [ ] Ext2
- [ ] Fat32
- [ ] ISO9660

### Userspace
- [x] System calls
- [ ] ELF
- [ ] Userspace
- [ ] Libc
- [ ] Bash
- [ ] Coreutils

### Network
- [ ] ARP
- [ ] ICMP
- [ ] TCP
- [ ] UDP

### USB
- [ ] OHCI/UHCI
- [ ] EHCI
- [ ] XHCI

### Other
- [ ] GUI (Window manager)

## Building And Running

Make sure you have following programs installed:
* Clang
* lld
* LLVM
* Make
* Nasm
* Qemu x86-64
* Xorriso
* Wget
* Tar

If you have Debian based system (Ubuntu, linux mint, Pop_os! etc) you can install them with this command:</br>
```sudo apt install clang lld make nasm qemu-system-x86 xorriso wget tar```

Follow this steps to build and run the os
1. Clone this repo with:</br>
``git clone --single-branch --branch master --depth 1 https://github.com/ilobilo/kernel``

2. Go to the root directory of cloned repo and run:<br />
``make -j$(nproc --all)`` For UEFI</br>
``make bios -j$(nproc --all)`` For BIOS</br>

## Discord server
https://discord.gg/fM5GK3RpS7

## Resources used:
* Printf: https://github.com/eyalroz/printf
* Osdev wiki: https://wiki.osdev.org
* Osdev discord server: https://discord.gg/RnCtsqD
* PonchoOS: https://github.com/Absurdponcho/PonchoOS
* Poncho discord server: https://discord.gg/N2Dpwpu4qT
* LyreOS: https://github.com/lyre-os/lyre
* PolarisOS: https://github.com/NSG650/Polaris
