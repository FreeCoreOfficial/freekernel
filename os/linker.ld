OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

PHDRS {
  text PT_LOAD FLAGS (r x);
  data PT_LOAD FLAGS (r w);
}

SECTIONS
{
    /* load address: 1MiB (typical for simple kernels) */
    . = 1M;

    /* executable/read-only segment */
    .text : {
        *(.multiboot)
        *(.text*)
    } :text

    /* place readonly data in the executable segment */
    .rodata : {
        *(.rodata*)
    } :text

    /* read-write segment (data + bss) */
    .data : {
        *(.data*)
    } :data

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    } :data

    /DISCARD/ : { *(.eh_frame) }
}
