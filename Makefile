ROOTDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCEDIR := $(ROOTDIR)/source

all: $(SOURCEDIR)/lai $(SOURCEDIR)/stivale2.h $(ROOTDIR)/limine
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR)

bios: $(SOURCEDIR)/lai $(SOURCEDIR)/stivale2.h $(ROOTDIR)/limine
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) bios

vnc: $(SOURCEDIR)/lai $(SOURCEDIR)/stivale2.h $(ROOTDIR)/limine
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) vnc

$(SOURCEDIR)/lai:
	@echo "Downloading Lai"
	@git clone --single-branch --branch=master --depth=1 https://github.com/thomtl/lai $(SOURCEDIR)/lai/ || echo "\e[31mFailed to download Lai!\e[0m"

$(SOURCEDIR)/stivale2.h:
	@echo "Downloading stivale2.h"
	@wget -nc https://github.com/stivale/stivale/raw/master/stivale2.h -P $(SOURCEDIR) || echo "\e[31mFailed to download stivale2.h!\e[0m"

$(ROOTDIR)/limine:
	@echo "Downloading Limine"
	@git clone --single-branch --branch=latest-binary --depth=1 https://github.com/limine-bootloader/limine || echo "\e[31mFailed to download Limine!\e[0m"
	@$(MAKE) -sC $(ROOTDIR)/limine

clean:
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -f $(ROOTDIR)/log.txt

distclean:
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -rf $(ROOTDIR)/limine
	@rm -f $(SOURCEDIR)/stivale2.h
	@rm -rf $(SOURCEDIR)/lai
	@rm -f $(ROOTDIR)/log.txt