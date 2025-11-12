// Placeholder for a simple panic facility (Week 6 refinement)
void kernel_panic(const char* msg) {
    (void)msg;
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}
