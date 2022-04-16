ROOTDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCEDIR := $(ROOTDIR)/source

all: libs $(ROOTDIR)/limine $(ROOTDIR)/ilar
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR)

bios: libs $(ROOTDIR)/limine $(ROOTDIR)/ilar
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) bios

vnc: libs $(ROOTDIR)/limine $(ROOTDIR)/ilar
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) vnc

$(ROOTDIR)/limine:
	@echo "Downloading Limine"
	@git clone --single-branch --branch=v3.0-branch-binary --depth=1 https://github.com/limine-bootloader/limine || echo "\e[31mFailed to download Limine!\e[0m"
	$(MAKE) -sC $(ROOTDIR)/limine

$(ROOTDIR)/ilar:
	@echo "Downloading ILAR"
	@git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/ilar || echo "\e[31mFailed to download ILAR!\e[0m"
	$(MAKE) -sC $(ROOTDIR)/ilar

libs:
	@$(MAKE) -sC $(ROOTDIR)/extlibs

clean:
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -f $(ROOTDIR)/log.txt

distclean:
	@$(MAKE) -sC $(ROOTDIR)/extlibs clean
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -rf $(ROOTDIR)/limine $(ROOTDIR)/ilar $(ROOTDIR)/log.txt