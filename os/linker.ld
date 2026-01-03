/* repaired linker.ld - simple, ld-friendly layout */
ENTRY(_start)

SECTIONS
{
    /* load address: 1MiB */
    . = 1M;

    .multiboot : { *(.multiboot) }

    .text : { *(.text*) }

    .rodata : { *(.rodata*) }

    .data : { *(.data*) }

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    }

    /* === IMPORTANT: end of kernel image === */
    kernel_end = .;

    /DISCARD/ : { *(.eh_frame) }
}
