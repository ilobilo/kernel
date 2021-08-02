all: limine
	@make -C ./source

bios: limine
	@make -C ./source bios

limine:
	@git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
	@make -C ./limine

clean:
	@make -C ./source clean

distclean:
	@make -C ./source clean
	@rm -rf ./limine
