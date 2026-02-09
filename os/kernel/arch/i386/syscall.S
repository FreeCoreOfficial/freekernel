; kernel/arch/i386/syscall.S
BITS 32
global syscall_handler
extern syscall_dispatch

section .text
syscall_handler:
    cli
    ; ABI: eax=num, ebx=arg1, ecx=arg2, edx=arg3, esi=arg4, edi=arg5
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax

    call syscall_dispatch

    add esp, 24

    sti
    iretd
