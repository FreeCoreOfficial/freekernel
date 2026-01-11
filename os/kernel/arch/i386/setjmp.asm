[BITS 32]

global setjmp
global longjmp

; int setjmp(jmp_buf env)
setjmp:
    mov ecx, [esp + 4]  ; pointer to jmp_buf
    mov [ecx + 0], ebx
    mov [ecx + 4], esi
    mov [ecx + 8], edi
    mov [ecx + 12], ebp
    lea edx, [esp + 4]  ; stack pointer before return address
    mov [ecx + 16], edx
    mov edx, [esp]      ; return address (eip)
    mov [ecx + 20], edx
    xor eax, eax        ; return 0
    ret

; void longjmp(jmp_buf env, int val)
longjmp:
    mov ecx, [esp + 4]  ; pointer to jmp_buf
    mov eax, [esp + 8]  ; val
    test eax, eax
    jnz .skip
    inc eax             ; if val==0, return 1
.skip:
    mov ebx, [ecx + 0]
    mov esi, [ecx + 4]
    mov edi, [ecx + 8]
    mov ebp, [ecx + 12]
    mov esp, [ecx + 16]
    mov edx, [ecx + 20] ; eip
    jmp edx