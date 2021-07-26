# kernel
My first os written in cpp (Currently displays nothing)<br />
Contributors are welcome

# Building And Running

Make sure you have this programs installed:
* GCC
* G++
* Make
* Nasm
* Qemu x86-64

1. First download toolchain from releases page

2. extract it in /opt directory with:
``cd /opt && sudo tar xpJf <Downloaded toolchain.tar.xz>``<br />
(bin directory should be /opt/x86_64-pc-elf/bin/)

3. add this line to your ~/.bashrc:
``export PATH="/opt/x86_64-pc-elf/bin:$PATH"``

4. Run ``source ~/.bashrc``

5. Clone this repo with:
``git clone https://github.com/ilobilo/kernel``

6. Go to kernel directory and run:
``make``

This command will download limine, compile it, build the kernel and run it in qemu.
