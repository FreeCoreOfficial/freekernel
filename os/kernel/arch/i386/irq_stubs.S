; kernel/arch/i386/irq_stubs.S
extern irq_handler

global irq_common_stub

; Macro pentru IRQ-uri
; %1 = Index IRQ (0-15)
; %2 = Remapped INT number (32-47)
%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push byte 0      ; Dummy error code
    push byte %2     ; INT number (ex: 32 pentru IRQ0)
    jmp irq_common_stub
%endmacro

IRQ   0,    32
IRQ   1,    33
IRQ   2,    34
IRQ   3,    35
IRQ   4,    36
IRQ   5,    37
IRQ   6,    38
IRQ   7,    39
IRQ   8,    40
IRQ   9,    41
IRQ  10,    42
IRQ  11,    43
IRQ  12,    44
IRQ  13,    45
IRQ  14,    46
IRQ  15,    47

irq_common_stub:
    pusha           ; Salvează registrele generale (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    
    mov ax, ds
    push eax        ; Salvează DS
    
    mov ax, 0x10    ; Încarcă Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp        ; Argument: pointer la registers_t
    call irq_handler
    add esp, 4      ; Curăță argumentul de pe stivă
    
    pop eax         ; Restaurează DS vechi
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa            ; Restaurează registrele generale
    add esp, 8      ; Curăță int_no și err_code
    iretd           ; Revine din întrerupere