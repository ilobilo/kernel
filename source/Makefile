KERNEL := kernel.bin
SOURCEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BACKGROUND := $(SOURCEDIR)/bg.bmp
INITRD := $(SOURCEDIR)/initrd.tar.gz

CC = x86_64-pc-elf-gcc
CPP = x86_64-pc-elf-g++
LD = x86_64-pc-elf-ld
AS = nasm

CFLAGS = -O3 -pipe
CPPFLAGS = -O3 -pipe
LDFLAGS = 
ASMFLAGS = 

INTERNALLDFLAGS :=             \
	-T$(SOURCEDIR)/linker.ld \
	-nostdlib              \
	-zmax-page-size=0x1000 \
	-static                \
	-pie                   \
	--no-dynamic-linker    \
	-ztext

INTERNALCFLAGS  :=           \
	-I$(SOURCEDIR)       \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fpie       \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone

INTERNALCPPFLAGS  :=         \
	-I$(SOURCEDIR)       \
	-std=c++14           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fpie       \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone

CFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.c')
CPPFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.cpp')
ASMFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.asm')
OBJ = $(CFILES:.c=.o)
OBJ += $(CPPFILES:.cpp=.o)
OBJ += $(ASMFILES:.asm=_asm.o)

.PHONY: all

all: $(KERNEL) initrd iso clean run

bios: $(KERNEL) initrd iso clean run-bios

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

%_asm.o: %.asm
	$(AS) $(ASMFLAGS) $^ -f elf64 -o $@

initrd:
	@tar czf $(INITRD) -C $(SOURCEDIR)/initramfs/ ./

iso:
	cp $(KERNEL) $(BACKGROUND) $(INITRD) $(SOURCEDIR)/../limine.cfg $(SOURCEDIR)/../limine/limine.sys \
		$(SOURCEDIR)/../limine/limine-cd.bin $(SOURCEDIR)/../limine/limine-eltorito-efi.bin $(SOURCEDIR)/../iso_root/

	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(SOURCEDIR)/../iso_root -o $(SOURCEDIR)/../image.iso > /dev/null

	$(SOURCEDIR)/../limine/limine-install $(SOURCEDIR)/../image.iso > /dev/null

clean:
	rm -rf $(KERNEL) $(OBJ) $(INITRD) $(SOURCEDIR)/../iso_root/*

run:
	qemu-system-x86_64 -enable-kvm -cpu max -smp 12 -m 512M -rtc base=localtime -serial stdio -cdrom $(SOURCEDIR)/../image.iso -drive if=pflash,format=raw,unit=0,file=$(SOURCEDIR)/../ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(SOURCEDIR)/../ovmf/OVMF_VARS-pure-efi.fd

run-bios:
	qemu-system-x86_64 -enable-kvm -cpu max -smp 12 -m 512M -rtc base=localtime -serial stdio -cdrom $(SOURCEDIR)/../image.iso -boot d
