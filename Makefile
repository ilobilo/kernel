KERNELDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: limine stivale2.h
	@make -s -C $(KERNELDIR)/source

bios: limine stivale2.h
	@make -s -C $(KERNELDIR)/source bios

stivale2.h:
	@wget -nc https://github.com/stivale/stivale/raw/master/stivale2.h -P $(KERNELDIR)/source/

limine:
	@git clone https://github.com/limine-bootloader/limine.git --single-branch --branch=v2.0-branch-binary --depth=1
	@make -C $(KERNELDIR)/limine

clean:
	@make -s -C $(KERNELDIR)/source clean

distclean:
	@make -s -C $(KERNELDIR)/source clean
	@rm -rf $(KERNELDIR)/limine
	@rm -f $(KERNELDIR)/source/stivale2.h
