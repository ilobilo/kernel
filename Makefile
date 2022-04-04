ROOTDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCEDIR := $(ROOTDIR)/source

all: libs limine
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR)

bios: libs limine
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) bios

vnc: libs limine
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) vnc

limine:
	@echo "Downloading Limine"
	@git clone --single-branch --branch=v3.0-branch-binary --depth=1 https://github.com/limine-bootloader/limine || echo "\e[31mFailed to download Limine!\e[0m"
	$(MAKE) -sC $(ROOTDIR)/limine

libs:
	@$(MAKE) -sC $(ROOTDIR)/extlibs

clean:
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -f $(ROOTDIR)/log.txt

distclean:
	@$(MAKE) -sC $(ROOTDIR)/extlibs clean
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -rf $(ROOTDIR)/limine
	@rm -f $(ROOTDIR)/log.txt