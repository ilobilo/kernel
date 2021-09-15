# Kernel project
My first os built from scratch<br />
Contributors are welcome

# [LICENSE](LICENSE)

# TODO

- [x] GDT
- [x] IDT
- [x] Keyboard
- [x] PIT
- [x] RTC
- [x] Iinitrd
- [x] Drawing on framebuffer
- [x] Serial debugging
- [ ] VFS
- [ ] Paging
- [ ] Heap
- [ ] PMM
- [ ] VMM
- [x] ACPI
- [ ] HPET
- [x] PCI
- [ ] AHCI
- [ ] Real filesystems (ext2, fat...)
- [ ] Multithreading
- [ ] TSS
- [ ] Userspace
- [ ] System calls
- [ ] POSIX
- [ ] ELF execution
- [ ] Port libc (Newlib)
- [ ] Port apps
- [ ] Sound
- [ ] Network
- [ ] USB
- [ ] GUI

# Building And Running

Make sure you have following programs installed:
* GCC
* G++
* Make
* Nasm
* Qemu x86-64
* Xorriso
* Wget
* Tar

1. Download the toolchain from releases page

2. Extract it in /opt directory with:<br />
``cd /opt && sudo tar xpJf <Downloaded toolchain.tar.xz>``<br />
(bin directory should be /opt/x86_64-pc-elf/bin/)

3. Add this line to your ~/.bashrc:<br />
``export PATH="/opt/x86_64-pc-elf/bin:$PATH"``

4. Run<br />``source ~/.bashrc``

5. Clone this repo with:<br />
``git clone --single-branch --branch master https://github.com/ilobilo/kernel``

6. Go to kernel directory and run:<br />
``make`` For UEFI<br />
``make bios`` For BIOS<br />

This command will download limine installer, compile it, build the kernel, create iso file and run it in qemu.

# Discord server
https://discord.gg/fM5GK3RpS7

# Resources used:
* Printf: https://github.com/eyalroz/printf
* Osdev wiki: https://wiki.osdev.org/
* Osdev discord server: https://discord.gg/RnCtsqD
* PonchoOS: https://github.com/Absurdponcho/PonchoOS
* Poncho discord server: https://discord.gg/N2Dpwpu4qT
* FaruOS (colours): https://github.com/leapofazzam123/faruos
* And of course GooACPI
