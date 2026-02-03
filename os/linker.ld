ENTRY(_start)

SECTIONS
{
    /* load address: 1MiB */
    . = 1M;

    .text :
    {
        KEEP(*(.multiboot))
        *(.text*)
    }

    .rodata : { *(.rodata*) }

    .data : { *(.data*) }

    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)

        /* Reserved Kernel Heap (32MB) inside BSS so Bootloader respects it */
        . = ALIGN(0x1000);
        __heap_start = .;
        . += 0x2000000;
        __heap_end = .;
    }

    kernel_end = .;

    kernel_end = .;

    /DISCARD/ : { *(.eh_frame) }
}
