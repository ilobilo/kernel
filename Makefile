KERNELDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: limine stivale2.h
	@$(MAKE) -s -C $(KERNELDIR)/source

bios: limine stivale2.h
	@$(MAKE) -s -C $(KERNELDIR)/source bios

test: limine stivale2.h
	@$(MAKE) -s -C $(KERNELDIR)/source test

stivale2.h:
	@wget -nc https://github.com/stivale/stivale/raw/master/stivale2.h -P $(KERNELDIR)/source/

limine:
	@git clone https://github.com/limine-bootloader/limine.git --single-branch --branch=latest-binary --depth=1
	@$(MAKE) -C $(KERNELDIR)/limine

clean:
	@$(MAKE) -s -C $(KERNELDIR)/source clean

distclean:
	@$(MAKE) -s -C $(KERNELDIR)/source clean
	@rm -rf $(KERNELDIR)/limine
	@rm -f $(KERNELDIR)/source/stivale2.h