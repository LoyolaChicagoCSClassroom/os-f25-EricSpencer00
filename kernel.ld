/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(main)
OUTPUT_FORMAT(elf32-i386)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
    /* link the multiboot struct here */
    . = 0;
    .multiboot : { *(.multiboot) }
    /* Begin putting sections at 1 MiB, a conventional place for kernels to be
       loaded at by the bootloader. */
    . = 1M;
    . = ALIGN(8);
    .text : { *(.text) }
    .rodata : { *(.rodata) }

    . = ALIGN(4096);
    _start_data = .;
    .data : { *(.data) }
    _end_data = .;
    . = ALIGN(4096);
    _start_bss = . ;
    .bss  : { *(.bss)  }
    _end_bss = ADDR(.bss) + SIZEOF(.bss) ;
    
    . = ALIGN(4096);

    /* Reserve a 16 KiB kernel stack */
    _start_stack = .;
    .stack : {
        . = . + 16K;
    }
    _end_stack = .;
    _end_kernel = .;
}
