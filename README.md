# Kernel project
My first os built from scratch<br />
Contributors are welcome

## [LICENSE](LICENSE)

## TODO

- [x] GDT
- [x] IDT
- [x] PS2 Keyboard
- [x] PS2 Mouse
- [x] PIT
- [x] RTC
- [x] Initrd
- [x] Drawing on framebuffer
- [x] Serial debugging
- [x] Page Frame allocator
- [x] Page Table Manager
- [x] Heap
- [x] VFS
- [x] DEVFS
- [x] ACPI
- [x] Shutdown
- [x] Reboot
- [x] PCI
- [x] HPET
- [x] VMWare Tools
- [ ] Sound
- [ ] AHCI
- [ ] Ext2
- [ ] Fat
- [ ] Threading
- [ ] Scheduling
- [x] TSS
- [x] SMP
- [ ] Userspace
- [x] System calls
- [ ] ELF
- [ ] Libc
- [ ] Bash
- [ ] Coreutils
- [ ] Network
- [ ] USB
- [ ] USB Keyboard
- [ ] USB Mouse
- [ ] GUI (Window manager)

## Building And Running

Make sure you have following programs installed:
* Clang
* lld
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
