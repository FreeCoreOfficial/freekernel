[BITS 32]

; -----------------------
; ISRs (Exceptions) Globals
; -----------------------
global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

; -----------------------
; IRQs Globals
; -----------------------
global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

extern isr_handler
extern irq_handler

; -----------------------
; ISR Macros (Exceptions)
; -----------------------
; Macro pentru excepții FĂRĂ cod de eroare (push dummy 0)
%macro ISR_NOERRCODE 1
  isr%1:
    cli
    push byte 0    ; dummy error code
    push byte %1   ; interrupt number
    jmp isr_common_stub
%endmacro

; Macro pentru excepții CU cod de eroare (CPU pune deja err_code)
%macro ISR_ERRCODE 1
  isr%1:
    cli
    push byte %1   ; interrupt number
    jmp isr_common_stub
%endmacro

; Definirea celor 32 de excepții standard x86
ISR_NOERRCODE 0   ; Divide by Zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; NMI
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; Bound Range Exceeded
ISR_NOERRCODE 6   ; Invalid Opcode
ISR_NOERRCODE 7   ; Device Not Available
ISR_ERRCODE   8   ; Double Fault
ISR_NOERRCODE 9   ; Coprocessor Segment Overrun
ISR_ERRCODE   10  ; Invalid TSS
ISR_ERRCODE   11  ; Segment Not Present
ISR_ERRCODE   12  ; Stack-Segment Fault
ISR_ERRCODE   13  ; General Protection Fault
ISR_ERRCODE   14  ; Page Fault
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; x87 Floating-Point Exception
ISR_ERRCODE   17  ; Alignment Check
ISR_NOERRCODE 18  ; Machine Check
ISR_NOERRCODE 19  ; SIMD Floating-Point Exception
ISR_NOERRCODE 20  ; Virtualization Exception
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30  ; Security Exception
ISR_NOERRCODE 31

; -----------------------
; IRQ Macro
; -----------------------
%macro IRQ 2
  irq%1:
    cli
    push byte 0
    push byte %2
    jmp irq_common_stub
%endmacro

IRQ   0, 32
IRQ   1, 33
IRQ   2, 34
IRQ   3, 35
IRQ   4, 36
IRQ   5, 37
IRQ   6, 38
IRQ   7, 39
IRQ   8, 40
IRQ   9, 41
IRQ  10, 42
IRQ  11, 43
IRQ  12, 44
IRQ  13, 45
IRQ  14, 46
IRQ  15, 47

; -----------------------
; Common ISR handler (Exceptions)
; -----------------------
isr_common_stub:
    pusha                ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10         ; Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp             ; Pointer la structura registers_t
    call isr_handler     ; Apelăm handler-ul C++
    add esp, 4           ; Curățăm argumentul (pointerul)

    pop gs
    pop fs
    pop es
    pop ds

    popa                 ; Restore registers
    add esp, 8           ; Curățăm int_no și err_code
    iretd

; -----------------------
; Common IRQ handler
; -----------------------
irq_common_stub:
    pusha

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10         ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp             ; pointer to registers_t
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa
    add esp, 8           ; pop int_no + err_code
  ;  sti
    iretd
