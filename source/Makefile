KERNEL := kernel.bin
SOURCEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
#FONTFILE := $(SOURCEDIR)/unifont.raw
BACKGROUND := $(SOURCEDIR)/bg.bmp

CC = x86_64-pc-elf-gcc
CPP = x86_64-pc-elf-g++
AS = nasm

CFLAGS = -O3 -pipe
CPPFLAGS = -O3 -pipe
ASMFLAGS = 

INTERNALLDFLAGS :=            \
	-fno-pic -fpie        \
	-Wl,-static,-pie,--no-dynamic-linker,-ztext \
	-static-pie           \
	-nostdlib             \
	-T$(SOURCEDIR)/linker.ld    \
	-z max-page-size=0x1000

INTERNALCFLAGS  :=            \
	-I.                   \
	-std=gnu11            \
	-ffreestanding        \
	-fno-stack-protector  \
	-fno-pic -fpie        \
	-mno-80387            \
	-mno-mmx              \
	-mno-3dnow            \
	-mno-sse              \
	-mno-sse2             \
	-mno-red-zone

INTERNALCPPFLAGS  :=          \
	-I.                   \
	-std=c++14            \
	-ffreestanding        \
	-fno-stack-protector  \
	-fno-pic -fpie        \
	-mno-80387            \
	-mno-mmx              \
	-mno-3dnow            \
	-mno-sse              \
	-mno-sse2             \
	-mno-red-zone

ASMFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.asm')
CPPFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.cpp')
CFILES = $(shell find $(SOURCEDIR)/ -type f -name '*.c')
OBJ = $(CFILES:.c=.o)
OBJ += $(CPPFILES:.cpp=.o)
OBJ += $(ASMFILES:.asm=_asm.o)

.PHONY: all

all: $(KERNEL) iso clean run
	@echo Done!

bios: $(KERNEL) iso clean run-bios
	@echo Done!

$(KERNEL): $(OBJ)
	@$(CC) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	@$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.cpp
	@$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

%_asm.o: %.asm
	@$(AS) $(ASMFLAGS) $^ -f elf64 -o $@

iso:
	@cp $(KERNEL) $(BACKGROUND) $(SOURCEDIR)/../limine.cfg $(SOURCEDIR)/../limine/limine.sys \
		$(SOURCEDIR)/../limine/limine-cd.bin $(SOURCEDIR)/../limine/limine-eltorito-efi.bin $(SOURCEDIR)/../iso_root/

	@xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(SOURCEDIR)/../iso_root -o $(SOURCEDIR)/../image.iso > /dev/null

	@$(SOURCEDIR)/../limine/limine-install $(SOURCEDIR)/../image.iso > /dev/null

clean:
	@rm -rf $(KERNEL) $(OBJ) $(SOURCEDIR)/../iso_root/*

run:
	@qemu-system-x86_64 -cpu max -smp 12 -cdrom $(SOURCEDIR)/../image.iso -drive if=pflash,format=raw,unit=0,file=$(SOURCEDIR)/../ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(SOURCEDIR)/../ovmf/OVMF_VARS-pure-efi.fd > /dev/null

run-bios:
	@qemu-system-x86_64 -cpu max -smp 12 -cdrom $(SOURCEDIR)/../image.iso -boot d > /dev/null
