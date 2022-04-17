LIBDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ROOTDIR := $(shell dirname $(LIBDIR))

all: $(LIBDIR)/lai $(LIBDIR)/cwalk $(LIBDIR)/limine.h $(LIBDIR)/cxxshim $(LIBDIR)/scalable-font2

$(LIBDIR)/lai:
	echo "Downloading lai"
	git clone --single-branch --branch=master --depth=1 https://github.com/thomtl/lai || echo "\e[31mFailed to download lai!\e[0m"

$(LIBDIR)/cwalk:
	echo "Downloading cwalk"
	git clone --single-branch --branch=master --depth=1 https://github.com/likle/cwalk || echo "\e[31mFailed to download cwalk!\e[0m"

$(LIBDIR)/limine.h:
	echo "Downloading limine.h"
	wget -nc https://raw.githubusercontent.com/limine-bootloader/limine/trunk/limine.h || echo "\e[31mFailed to download limine.h!\e[0m"

$(LIBDIR)/cxxshim:
	echo "Downloading cxxshim"
	git clone --single-branch --branch=master --depth=1 https://github.com/managarm/cxxshim || echo "\e[31mFailed to download cxxshim!\e[0m"

$(LIBDIR)/scalable-font2:
	echo "Downloading scalable-font2"
	git clone --single-branch --branch=master --depth=1 https://gitlab.com/bztsrc/scalable-font2 || echo "\e[31mFailed to download ssfn!\e[0m"

clean:
	rm -rf $(LIBDIR)/lai $(LIBDIR)/cwalk $(LIBDIR)/limine.h $(LIBDIR)/cxxshim $(LIBDIR)/scalable-font2