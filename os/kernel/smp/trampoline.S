/*
 * SMP Trampoline Code
 * This code is loaded at a low physical address (e.g., 0x7000) and is executed
 * by Application Processors (APs) when they receive a SIPI.
 */

.set TRAMPOLINE_ADDR, 0x7000
.org TRAMPOLINE_ADDR

.code16
.global trampoline_start
trampoline_start:
    cli
    
    // Disable paging (APs start with paging from BSP)
    mov %cr0, %eax
    and $0x7FFFFFFF, %eax
    mov %eax, %cr0

    // Load GDT
    lgdt gdt_desc

    // Enter 32-bit Protected Mode
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    // Far jump to flush pipeline and load CS with 32-bit code segment
    ljmp $0x08, $protected_mode

.code32
protected_mode:
    // Setup data segments
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    // Each AP needs its own stack. We'll pre-allocate them.
    // This is a simple example; a real OS would have a more robust way
    // to assign stacks. We use the APIC ID to get a unique stack.
    // Read APIC ID (requires LAPIC to be mapped at a known address,
    // but it's not mapped yet for the AP). We'll do this in C.
    // For now, let's just call the C function.
    extern ap_main
    call ap_main

    // Should not return from ap_main
ap_hang:
    hlt
    jmp ap_hang

.global gdt_desc
gdt_desc:
    .word 0 // limit (filled by C code)
    .long 0 // base (filled by C code)

.global ap_stack_top
ap_stack_top:
    .long 0 // Filled by C code