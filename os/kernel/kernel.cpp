extern "C" void idt_init();

extern "C" void kernel_main() {
    idt_init();
    asm volatile("sti");

    while (1)
        asm volatile("hlt");
}
