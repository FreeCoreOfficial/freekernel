; kernel/arch/i386/syscall.S  -- NASM syntax (elf32)
BITS 32
extern panic
global syscall_handler
section .text

syscall_handler:
    cli
    push dword syscall_handler_msg
    call panic
    add esp, 4
    hlt

section .rodata
syscall_handler_msg:
    db "Syscall invoked but not implemented", 0
