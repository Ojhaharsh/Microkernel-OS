#include <stdint.h>
#include "interrupts.h"

// IDT entry (64-bit interrupt gate)
typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t  idtp;

extern void isr_irq0(void);
extern void isr_irq1(void);
extern void isr_irq4(void);  // Serial (COM1)
extern void isr_syscall(void);
extern void isr_gp(void);
extern void isr_pf(void);
extern void isr_ts(void);
extern void isr_ss(void);
extern void isr_np(void);
extern void isr_df(void);
extern void isr_nm(void);
extern void isr_ud(void);
extern void isr_de(void);
extern void isr_br(void);
extern void isr_of(void);
extern void isr_bp(void);
extern void isr_ac(void);
extern void isr_mf(void);
extern void isr_db(void);
extern void isr_nmi(void);
extern void isr_res15(void);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret; __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}

static void idt_set_gate(int n, void* isr, uint8_t flags) {
    uint64_t addr = (uint64_t)(uintptr_t)isr;
    idt[n].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[n].selector    = 0x08; // kernel code segment
    idt[n].ist         = 0;
    idt[n].type_attr   = flags; // 0x8E = present, DPL=0, type=interrupt gate
    idt[n].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[n].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[n].zero        = 0;
}

static void idt_set_gate_ist(int n, void* isr, uint8_t flags, uint8_t ist) {
    idt_set_gate(n, isr, flags);
    idt[n].ist = ist & 0x7;
}

void interrupts_init(void) {
    extern void isr_any(void);
    // PIC remap: master to 0x20, slave to 0x28
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    // Unmask IRQ0 (timer), IRQ1 (keyboard), and IRQ4 (serial) on master; mask all on slave
    outb(0x21, 0xE8); // 0xE8 = 11101000b (unmask IRQ0, IRQ1, IRQ4)
    outb(0xA1, 0xFF);

    // IDT pointer
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)(uintptr_t)&idt[0];

    // Zero IDT and set generic handlers for exceptions
    for (int i = 0; i < 256; ++i) {
        idt[i].offset_low = 0; idt[i].selector = 0; idt[i].ist = 0; idt[i].type_attr = 0;
        idt[i].offset_mid = 0; idt[i].offset_high = 0; idt[i].zero = 0;
    }
    for (int v = 0; v < 32; ++v) {
        idt_set_gate(v, isr_any, 0x8E);
    }

    // Exceptions to aid debugging user-mode issues, use IST1 for robustness
    idt_set_gate_ist(8,  isr_df, 0x8E, 1);
    idt_set_gate_ist(7,  isr_nm, 0x8E, 1);
    idt_set_gate_ist(6,  isr_ud, 0x8E, 1);
    idt_set_gate_ist(0,  isr_de, 0x8E, 1);
    idt_set_gate_ist(1,  isr_db, 0x8E, 1);
    idt_set_gate_ist(2,  isr_nmi, 0x8E, 1);
    idt_set_gate_ist(5,  isr_br, 0x8E, 1);
    idt_set_gate_ist(4,  isr_of, 0x8E, 1);
    idt_set_gate_ist(3,  isr_bp, 0x8E, 1);
    idt_set_gate_ist(17, isr_ac, 0x8E, 1);
    idt_set_gate_ist(16, isr_mf, 0x8E, 1);
    idt_set_gate_ist(15, isr_res15, 0x8E, 1);
    idt_set_gate_ist(13, isr_gp, 0x8E, 1);
    idt_set_gate_ist(14, isr_pf, 0x8E, 1);
    idt_set_gate_ist(10, isr_ts, 0x8E, 1);
    idt_set_gate_ist(11, isr_np, 0x8E, 1);
    idt_set_gate_ist(12, isr_ss, 0x8E, 1);
    // Set IRQ0 gate (DPL=0)
    idt_set_gate(0x20, isr_irq0, 0x8E);
    // Set IRQ1 gate (keyboard)
    idt_set_gate(0x21, isr_irq1, 0x8E);
    // Set IRQ4 gate (serial COM1)
    idt_set_gate(0x24, isr_irq4, 0x8E);
    // Set syscall gate int 0x80 with DPL=3 (interrupt gate clears IF)
    idt_set_gate(0x80, isr_syscall, 0xEE);

    // lidt
    __asm__ __volatile__("lidt %0" : : "m"(idtp));
}

void pit_init(uint32_t hz) {
    if (hz == 0) hz = 100;
    uint32_t divisor = 1193182 / hz;
    outb(0x43, 0x36); // channel 0, lo/hi, mode 3 (square wave)
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

// Exposed to ISR stub
void scheduler_tick(void); // from scheduler.c
void keyboard_irq_handler(void); // from keyboard.c

void timer_isr(void) {
    scheduler_tick();
}

void keyboard_irq(void) {
    keyboard_irq_handler();
}

void serial_irq(void) {
    serial_irq_handler();
}
