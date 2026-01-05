; kernel/arch/i386/isr_default.S
; Default ISR fallback — should NEVER return
; NASM syntax, elf32

BITS 32

extern panic
global isr_default

section .text
isr_default:
    cli                 ; stop interrupts
    pushad              ; save registers (for panic backtrace, dacă vrei)

    push dword isr_msg  ; panic(const char*)
    call panic

.hang:
    cli
    hlt
    jmp .hang           ; safety: panic() MUST not return

section .rodata
isr_msg:
    db "Unhandled interrupt (default handler)", 0
