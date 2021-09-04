# kernel
My first os built from scratch<br />
Contributors are welcome

# [LICENSE](LICENSE)

# TODO

- [x] GDT
- [x] IDT
- [x] Keyboard
- [x] Iinitrd
- [ ] Serial
- [ ] Heap
- [ ] PMM
- [ ] VMM
- [ ] ACPI
- [ ] PCI
- [ ] AHCI
- [ ] Real filesystems
- [ ] Multithreading
- [ ] Userspace
- [ ] POSIX
- [ ] Port libc
- [ ] Port apps
- [ ] Sound
- [ ] Network
- [ ] USB
- [ ] GUI

# Building And Running

Make sure you have this programs installed:
* GCC
* G++
* Make
* Nasm
* Qemu x86-64
* Wget

1. Download the toolchain from releases page

2. extract it in /opt directory with:<br />
``cd /opt && sudo tar xpJf <Downloaded toolchain.tar.xz>``<br />
(bin directory should be /opt/x86_64-pc-elf/bin/)

3. add this line to your ~/.bashrc:<br />
``export PATH="/opt/x86_64-pc-elf/bin:$PATH"``

4. Run<br />``source ~/.bashrc``

5. Clone this repo with:<br />
``git clone --single-branch --branch master https://github.com/ilobilo/kernel``

6. Go to kernel directory and run:<br />
``make/make bios``

This command will download limine, compile it, build the kernel and run it in qemu.

# Discord server
https://discord.gg/fM5GK3RpS7
