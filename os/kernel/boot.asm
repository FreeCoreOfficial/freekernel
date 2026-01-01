; kernel/boot.asm
BITS 32

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000000
    dd -(0x1BADB002)

section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    cli

    ; setup stack
    mov esp, stack_top

    ; GRUB passes:
    ; eax = magic
    ; ebx = multiboot info ptr

    push ebx             ; arg2
    push eax             ; arg1
    call kernel_main

.hang:
    hlt
    jmp .hang
