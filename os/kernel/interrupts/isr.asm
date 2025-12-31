global irq0
global irq1

extern irq_handler

irq0:
    pusha
    call irq_handler
    popa
    iretd

irq1:
    pusha
    call irq_handler
    popa
    iretd
