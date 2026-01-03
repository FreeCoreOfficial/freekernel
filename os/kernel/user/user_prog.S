BITS 32
GLOBAL _start

SECTION .text
ORG 0x08048000

_start:
    mov eax, 1          ; syscall write
    mov ebx, 1          ; fd = stdout
    mov ecx, msg
    mov edx, msg_len
    int 0x80

    xor ebx, ebx
    mov eax, 2          ; syscall exit
    int 0x80

msg:
    db "hello from userland", 0xA
msg_len equ $ - msg
