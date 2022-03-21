LIBDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ROOTDIR := $(shell dirname $(LIBDIR))

all: lai stivale2.h cxxshim

lai:
	echo "Downloading lai"
	git clone --single-branch --branch=master --depth=1 https://github.com/thomtl/lai || echo "\e[31mFailed to download lai!\e[0m"

stivale2.h:
	echo "Downloading stivale2.h"
	wget -nc https://github.com/stivale/stivale/raw/master/stivale2.h || echo "\e[31mFailed to download stivale2.h!\e[0m"

cxxshim:
	echo "Downloading cxxshim"
	git clone --single-branch --branch=master --depth=1 https://github.com/managarm/cxxshim || echo "\e[31mFailed to download cxxshim!\e[0m"

clean:
	rm -rf $(LIBDIR)/lai $(LIBDIR)/stivale2.h $(LIBDIR)/cxxshim