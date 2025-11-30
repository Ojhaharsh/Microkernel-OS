// Simple serial (COM1) helpers used across the kernel
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_putc(char c) {
    const uint16_t COM1 = 0x3F8;
    while (!(inb(COM1 + 5) & 0x20)) { /* wait */ }
    outb(COM1, (uint8_t)c);
}

void serial_write(const char* s) {
    for (; *s; ++s) serial_putc(*s);
}
