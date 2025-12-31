global idt_load
global isr_stub

extern isr_handler

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr_stub:
    pusha
    call isr_handler
    popa
    iretd
