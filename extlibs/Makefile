EXTLIBDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ROOTDIR := $(shell dirname $(EXTLIBDIR))

all: $(EXTLIBDIR)/lai $(EXTLIBDIR)/ilar $(EXTLIBDIR)/cwalk $(EXTLIBDIR)/printf $(EXTLIBDIR)/limine $(EXTLIBDIR)/cxxshim $(EXTLIBDIR)/scalable-font2

$(EXTLIBDIR)/lai:
	echo "Downloading lai"
	git clone --single-branch --branch=master --depth=1 https://github.com/managarm/lai || echo "\e[31mFailed to download lai!\e[0m"

$(EXTLIBDIR)/ilar:
ifdef USEILAR
	@echo "Downloading ilar"
	@git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/ilar || echo "\e[31mFailed to download ilar!\e[0m"
	$(MAKE) -sC $(EXTLIBDIR)/ilar
endif

$(EXTLIBDIR)/cwalk:
	echo "Downloading cwalk"
	git clone --single-branch --branch=master --depth=1 https://github.com/likle/cwalk || echo "\e[31mFailed to download cwalk!\e[0m"

$(EXTLIBDIR)/printf:
	echo "Downloading printf"
	git clone --single-branch --branch=master --depth=1 https://github.com/eyalroz/printf || echo "\e[31mFailed to download printf!\e[0m"

$(EXTLIBDIR)/limine:
	@echo "Downloading limine"
	@git clone --single-branch --branch=v3.0-branch-binary --depth=1 https://github.com/limine-bootloader/limine || echo "\e[31mFailed to download limine!\e[0m"
	$(MAKE) -sC $(EXTLIBDIR)/limine

$(EXTLIBDIR)/cxxshim:
	echo "Downloading cxxshim"
	git clone --single-branch --branch=master --depth=1 https://github.com/managarm/cxxshim || echo "\e[31mFailed to download cxxshim!\e[0m"

$(EXTLIBDIR)/scalable-font2:
	echo "Downloading scalable-font2"
	git clone --single-branch --branch=master --depth=1 https://gitlab.com/bztsrc/scalable-font2 || echo "\e[31mFailed to download ssfn!\e[0m"

clean:
	rm -rf $(EXTLIBDIR)/lai $(EXTLIBDIR)/ilar $(EXTLIBDIR)/cwalk $(EXTLIBDIR)/printf $(EXTLIBDIR)/limine $(EXTLIBDIR)/cxxshim $(EXTLIBDIR)/scalable-font2