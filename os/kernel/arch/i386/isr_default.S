; kernel/arch/i386/isr_default.S  -- NASM syntax (elf32)
BITS 32
extern panic
global isr_default
section .text

isr_default:
    cli
    pusha
    push dword isr_msg
    call panic
    add esp, 4
    popa
    hlt

section .rodata
isr_msg:
    db "Unhandled interrupt (default handler)", 0
