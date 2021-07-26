ENTRY(_start)
 
SECTIONS
{
    . = 0xffffffff80200000;
 
    .stivale2hdr : {
        KEEP(*(.stivale2hdr))
    }
 
    .text : {
        *(.text*)
    }
 
    .rodata : {
        *(.rodata*)
    }
 
    .data : {
        *(.data*)
    }
 
    .bss : {
        *(COMMON)
        *(.bss*)
    }
}
