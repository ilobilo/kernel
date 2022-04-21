ROOTDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCEDIR := $(ROOTDIR)/source

all: libs
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR)

bios: libs
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) bios

vnc: libs
	@$(MAKE) -sC $(SOURCEDIR) clean
	@$(MAKE) -sC $(SOURCEDIR) vnc

libs:
	@$(MAKE) -sC $(ROOTDIR)/extlibs

clean:
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -f $(ROOTDIR)/log.txt

distclean: clean
	@$(MAKE) -sC $(ROOTDIR)/extlibs clean