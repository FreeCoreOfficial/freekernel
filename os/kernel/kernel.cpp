extern "C" void gdt_init();

extern "C" void kernel_main() {
    gdt_init();

    while (1)
        asm volatile("hlt");
}
