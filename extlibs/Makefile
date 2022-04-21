LIBDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ROOTDIR := $(shell dirname $(LIBDIR))

all: $(LIBDIR)/lai $(LIBDIR)/ilar $(LIBDIR)/cwalk $(LIBDIR)/printf $(LIBDIR)/limine $(LIBDIR)/cxxshim $(LIBDIR)/scalable-font2

$(LIBDIR)/lai:
	echo "Downloading lai"
	git clone --single-branch --branch=master --depth=1 https://github.com/thomtl/lai || echo "\e[31mFailed to download lai!\e[0m"

$(LIBDIR)/ilar:
ifdef USEILAR
	@echo "Downloading ilar"
	@git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/ilar || echo "\e[31mFailed to download ilar!\e[0m"
	$(MAKE) -sC $(LIBDIR)/ilar
endif

$(LIBDIR)/cwalk:
	echo "Downloading cwalk"
	git clone --single-branch --branch=master --depth=1 https://github.com/likle/cwalk || echo "\e[31mFailed to download cwalk!\e[0m"

$(LIBDIR)/printf:
	echo "Downloading printf"
	git clone --single-branch --branch=master --depth=1 https://github.com/eyalroz/printf || echo "\e[31mFailed to download printf!\e[0m"

$(LIBDIR)/limine:
	@echo "Downloading limine"
	@git clone --single-branch --branch=v3.0-branch-binary --depth=1 https://github.com/limine-bootloader/limine || echo "\e[31mFailed to download limine!\e[0m"
	$(MAKE) -sC $(LIBDIR)/limine

$(LIBDIR)/cxxshim:
	echo "Downloading cxxshim"
	git clone --single-branch --branch=master --depth=1 https://github.com/managarm/cxxshim || echo "\e[31mFailed to download cxxshim!\e[0m"

$(LIBDIR)/scalable-font2:
	echo "Downloading scalable-font2"
	git clone --single-branch --branch=master --depth=1 https://gitlab.com/bztsrc/scalable-font2 || echo "\e[31mFailed to download ssfn!\e[0m"

clean:
	rm -rf $(LIBDIR)/lai $(LIBDIR)/ilar $(LIBDIR)/cwalk $(LIBDIR)/printf $(LIBDIR)/limine $(LIBDIR)/cxxshim $(LIBDIR)/scalable-font2