SOURCEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BACKGROUND := $(SOURCEDIR)/misc/bg.bmp
FONT := $(SOURCEDIR)/misc/font.bin
SFNFONT := $(SOURCEDIR)/misc/font.sfn
INITRD := $(SOURCEDIR)/initrd.tar.gz
KERNEL := $(SOURCEDIR)/kernel.elf
ISO := $(SOURCEDIR)/../image.iso
DISK0 = $(SOURCEDIR)/../disk0.img
DISK1 = $(SOURCEDIR)/../disk1.img

VERSION = 0
NET_DEBUG = 1
LVL5_PAGING = 0

CC = clang
CPP = clang++
LD = ld.lld
ASM = nasm

CXXFILT = llvm-cxxfilt

LIMINE = $(SOURCEDIR)/../limine/limine-install
XORRISO = xorriso
QEMU = qemu-system-x86_64

QEMUFLAGS = -M q35 -serial stdio \
	-cpu max -smp 12 -m 512M \
	-boot d -rtc base=localtime \
	-drive format=raw,file=$(DISK0) \
	-audiodev id=audio,driver=alsa \
	-machine pcspk-audiodev=audio \
	-net nic,model=e1000 -net user \
	-net nic,model=rtl8139 -net user

XORRISOFLAGS = -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label

CFLAGS = -Ofast -pipe -Werror -Wall -Wextra \
	-DGIT_VERSION=\"$(shell git rev-parse --short HEAD)\" \
	-DKERNEL_VERSION=\"$(VERSION)\" -DNET_DEBUG=$(NET_DEBUG) -DLVL5_PAGING=$(LVL5_PAGING) -g

CPPFLAGS = $(CFLAGS) -Wno-c99-designator -Wno-unused-parameter

LDFLAGS = 
ASMFLAGS = -f elf64 -g

override INTERNALLDFLAGS :=  \
	-T$(SOURCEDIR)/linker.ld \
	-nostdlib                \
	-zmax-page-size=0x1000   \
	-static

override INTERNALCFLAGS :=     \
	-target x86_64-pc-none-elf \
	-I$(SOURCEDIR)/            \
	-std=gnu17                 \
	-ffreestanding             \
	-fno-exceptions            \
	-fno-stack-protector       \
	-fno-use-cxa-atexit        \
	-fno-omit-frame-pointer    \
	-fsanitize=undefined       \
	-fno-rtti                  \
	-fno-pic                   \
	-mabi=sysv                 \
	-mno-80387                 \
	-mno-mmx                   \
	-mno-3dnow                 \
	-mno-sse                   \
	-mno-sse2                  \
	-mno-red-zone              \
	-mcmodel=kernel

override INTERNALCPPFLAGS :=   \
	-target x86_64-pc-none-elf \
	-I$(SOURCEDIR)/            \
	-std=gnu++17               \
	-ffreestanding             \
	-fno-exceptions            \
	-fno-stack-protector       \
	-fno-use-cxa-atexit        \
	-fsanitize=undefined       \
	-fno-omit-frame-pointer    \
	-fno-rtti                  \
	-fno-pic                   \
	-mabi=sysv                 \
	-mno-80387                 \
	-mno-mmx                   \
	-mno-3dnow                 \
	-mno-sse                   \
	-mno-sse2                  \
	-mno-red-zone              \
	-mcmodel=kernel

override CFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.c' -not -path "$(SOURCEDIR)/initrd/*")
override CPPFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.cpp' -not -path "$(SOURCEDIR)/initrd/*")
override ASMFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.asm' -not -path "$(SOURCEDIR)/initrd/*")

override OBJ = $(CFILES:.c=.o)
override OBJ += $(CPPFILES:.cpp=.o)
override OBJ += $(ASMFILES:.asm=_asm.o)

.PHONY: all
all: $(KERNEL) initrd
	$(MAKE) iso
	$(MAKE) clean run

.PHONY: bios
bios: $(KERNEL) initrd
	$(MAKE) iso
	$(MAKE) clean run-bios

.PHONY: test
test: $(KERNEL) initrd
	$(MAKE) iso
	$(MAKE) clean run-test

$(KERNEL):
	$(MAKE) $(OBJ)
	@printf "LD\t%s\n" $(lastword $(subst /, ,$(KERNEL)))
	$(LD) $(LDFLAGS) $(INTERNALLDFLAGS) $(SFNFONT) $(OBJ) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(lastword $(subst /, ,$<))
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.cpp
	@printf "CPP\t%s\n" $(lastword $(subst /, ,$<))
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

%_asm.o: %.asm
	@printf "ASM\t%s\n" $(lastword $(subst /, ,$<))
	$(ASM) $(ASMFLAGS) $< -o $@

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
ifndef NOCLEAN
	rm -rf $(KERNEL) $(OBJ) $(HDR_DEPS) $(INITRD) $(SOURCEDIR)/../iso_root/* $(SOURCEDIR)/../null.d
endif

run:
	echo "\nBooting iso in uefi mode...\n"
ifndef DEBUG
	$(QEMU) $(QEMUFLAGS) -enable-kvm -cdrom $(ISO) -drive if=pflash,format=raw,unit=0,file=$(SOURCEDIR)/../ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(SOURCEDIR)/../ovmf/OVMF_VARS-pure-efi.fd | $(CXXFILT)
else
	$(QEMU) $(QEMUFLAGS) -no-reboot -no-shutdown -d int -D $(SOURCEDIR)/../log.txt -machine smm=off -cdrom $(ISO) -drive if=pflash,format=raw,unit=0,file=$(SOURCEDIR)/../ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(SOURCEDIR)/../ovmf/OVMF_VARS-pure-efi.fd | $(CXXFILT)
endif

run-bios:
	echo "\nBooting iso in bios mode...\n"
ifndef DEBUG
	$(QEMU) $(QEMUFLAGS) -enable-kvm -cdrom $(ISO) | $(CXXFILT)
else
	$(QEMU) $(QEMUFLAGS) -no-reboot -no-shutdown -d int -D $(SOURCEDIR)/../log.txt -machine smm=off -cdrom $(ISO) | $(CXXFILT)
endif

run-test:
	echo "\nBooting iso in bios mode... (testing)\n"
ifndef DEBUG
	$(QEMU) $(QEMUFLAGS) -vnc 0.0.0.0:1 -cdrom $(ISO) | $(CXXFILT)
else
	$(QEMU) $(QEMUFLAGS) -no-reboot -no-shutdown -vnc 0.0.0.0:1 -d int -D $(SOURCEDIR)/../log.txt -machine smm=off -cdrom $(ISO) | $(CXXFILT)
endif