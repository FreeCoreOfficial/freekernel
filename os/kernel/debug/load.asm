[BITS 32]
global debug_halt

debug_halt:
    cli
.hang:
    hlt
    jmp .hang
