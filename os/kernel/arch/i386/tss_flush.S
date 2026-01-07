[BITS 32]
global tss_flush

tss_flush:
    mov ax, 0x2B      ; Index 5 in GDT (5 * 8 = 40 = 0x28) | RPL 3 (0x03) = 0x2B
                      ; Sau pur și simplu 0x28 (Ring 0 access to TSS descriptor)
                      ; Descriptorul TSS trebuie accesat ca 0x28 (Index 5, TI=0, RPL=0)
    mov ax, 0x28      ; 5 * 8 = 40 = 0x28
    ltr ax            ; Load Task Register
    ret