global irq0
global irq1
global idt_load

extern irq_handler

; ======================
; LOAD IDT
; ======================
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; ======================
; IRQ 0 – timer
; ======================
irq0:
    pusha
    call irq_handler
    popa
    iretd

; ======================
; IRQ 1 – keyboard
; ======================
irq1:
    pusha
    call irq_handler
    popa
    iretd
