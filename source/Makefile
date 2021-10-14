SOURCEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BACKGROUND := $(SOURCEDIR)/bg.bmp
FONT := $(SOURCEDIR)/font.bin
INITRD := $(SOURCEDIR)/initrd.tar.gz
KERNEL := $(SOURCEDIR)/kernel.elf
ISO := $(SOURCEDIR)/../image.iso
DISK = $(SOURCEDIR)/../disk.img
VERSION = 0

LIMINE = $(SOURCEDIR)/../limine/limine-install
XORRISO = xorriso
QEMU = qemu-system-x86_64

QEMUFLAGS = -M q35 -enable-kvm \
	-cpu max -smp 12 -m 512M \
	-boot d -rtc base=localtime \
	-serial stdio -no-reboot -no-shutdown

XORRISOFLAGS = -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label

CC = x86_64-pc-elf-gcc
CPP = x86_64-pc-elf-g++
LD = x86_64-pc-elf-ld
AS = nasm

CW = -Wall -Wextra -Werror
CPPW = -Wall -Wextra -Werror \
	-Wno-write-strings -Wno-sized-deallocation \
	-Wno-int-to-pointer-cast -Wno-strict-aliasing \
	-Wno-unused-but-set-variable \
	-Wno-unused-variable -Wno-unused-parameter \
	-Wno-unknown-pragmas

CFLAGS = -O3 -pipe $(CW) -DGIT_VERSION=\"$(shell git rev-parse --short HEAD)\" -DKERNEL_VERSION=\"$(VERSION)\"
CPPFLAGS = -O3 -pipe $(CPPW) -DGIT_VERSION=\"$(shell git rev-parse --short HEAD)\" -DKERNEL_VERSION=\"$(VERSION)\"
LDFLAGS = 
ASMFLAGS = -f elf64

INTERNALLDFLAGS :=             \
	-T$(SOURCEDIR)/linker.ld \
	-nostdlib              \
	-zmax-page-size=0x1000 \
	-static                \
	-pie                   \
	--no-dynamic-linker    \
	-ztext

INTERNALCFLAGS  :=           \
	-I$(SOURCEDIR)/include   \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fpie       \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-red-zone        \
	-fno-exceptions

INTERNALCPPFLAGS  :=         \
	-I$(SOURCEDIR)/include   \
	-std=c++14           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fpie       \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-red-zone        \
	-fno-exceptions

CFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.c' -not -path "$(SOURCEDIR)/initrd/*")
CPPFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.cpp' -not -path "$(SOURCEDIR)/initrd/*")
ASMFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.asm' -not -path "$(SOURCEDIR)/initrd/*")
OBJ = $(CFILES:.c=.o)
OBJ += $(CPPFILES:.cpp=.o)
OBJ += $(ASMFILES:.asm=_asm.o)

.PHONY: all

all: $(KERNEL) initrd iso clean run

bios: $(KERNEL) initrd iso clean run-bios

$(KERNEL): $(OBJ)
	@printf "LD\t%s\n" $(lastword $(subst /, ,$(KERNEL)))
	$(LD) $(LDFLAGS) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(lastword $(subst /, ,$<))
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.cpp
	@printf "CPP\t%s\n" $(lastword $(subst /, ,$<))
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

%_asm.o: %.asm
	@printf "AS\t%s\n" $(lastword $(subst /, ,$<))
	$(AS) $(ASMFLAGS) $< -o $@

.PHONY: initrd
initrd:
	@printf "TAR\t%s\n" $(lastword $(subst /, ,$(INITRD)))
	tar czf $(INITRD) -C $(SOURCEDIR)/initrd/ ./ --format=ustar 

iso:
	cp $(KERNEL) $(BACKGROUND) $(FONT) $(INITRD) $(SOURCEDIR)/../limine.cfg $(SOURCEDIR)/../limine/limine.sys \
		$(SOURCEDIR)/../limine/limine-cd.bin $(SOURCEDIR)/../limine/limine-eltorito-efi.bin $(SOURCEDIR)/../iso_root/

	@printf "XORRISO\t%s\n" $(lastword $(subst /, ,$(ISO)))
	$(XORRISO) $(XORRISOFLAGS) $(SOURCEDIR)/../iso_root -o $(ISO) 2> /dev/null || echo "\e[31mFailed to build iso!\e[0m"

	@printf "LIMINE\t%s\n" $(lastword $(subst /, ,$(ISO)))
	$(LIMINE) $(ISO) 2> /dev/null || echo "\e[31mFailed to install limine!\e[0m"

clean:
	rm -rf $(KERNEL) $(OBJ) $(INITRD) $(SOURCEDIR)/../iso_root/*

run:
	echo "\nBooting iso in uefi mode...\n"
	$(QEMU) $(QEMUFLAGS) -cdrom $(ISO) -drive if=pflash,format=raw,unit=0,file=$(SOURCEDIR)/../ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(SOURCEDIR)/../ovmf/OVMF_VARS-pure-efi.fd

run-bios:
	echo "\nBooting iso in bios mode...\n"
	$(QEMU) $(QEMUFLAGS) -cdrom $(ISO)