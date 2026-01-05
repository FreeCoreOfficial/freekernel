OUTPUT_FORMAT(elf32-i386)
ENTRY(trampoline_start)

SECTIONS {
    . = 0x7000;
    .text : { *(.text) }
}