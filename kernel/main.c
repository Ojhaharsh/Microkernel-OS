#include <stdint.h>
#include <stddef.h>
#include "scheduler.h" /* Week 3 (not invoked yet) */
#include "memory.h"
#include "interrupts.h"
#include "syscall.h"
#include "ipc.h"
#include "keyboard.h"

/* Week 6 user-space demo: user entry shims */
static uint64_t ustack_server_top = 0, ustack_client_top = 0;
extern void enter_user_mode_syscall(uint64_t rip, uint64_t rsp);
void start_user_server(void* _){ (void)_; extern uint8_t user_server_start; enter_user_mode_syscall((uint64_t)(uintptr_t)&user_server_start, ustack_server_top); for(;;) { __asm__ __volatile__("hlt"); } }
void start_user_client(void* _){ (void)_; extern uint8_t user_client_start; enter_user_mode_syscall((uint64_t)(uintptr_t)&user_client_start, ustack_client_top); for(;;) { __asm__ __volatile__("hlt"); } }
void start_keyboard_test(void* _){ (void)_; extern uint8_t user_keyboard_test; enter_user_mode_syscall((uint64_t)(uintptr_t)&user_keyboard_test, ustack_client_top); for(;;) { __asm__ __volatile__("hlt"); } }
void start_user_shell(void* _){ (void)_; extern uint8_t user_shell_start; enter_user_mode_syscall((uint64_t)(uintptr_t)&user_shell_start, ustack_client_top); for(;;) { __asm__ __volatile__("hlt"); } }

/* Simple VGA text console (80x25, color attribute 0x0F: white on black) */
static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const size_t VGA_COLS = 80;
static const size_t VGA_ROWS = 25;
static uint8_t vga_color = 0x0F; /* white on black */
static size_t cursor_row = 0;
static size_t cursor_col = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void console_clear(void) {
    for (size_t r = 0; r < VGA_ROWS; ++r) {
        for (size_t c = 0; c < VGA_COLS; ++c) {
            VGA[r * VGA_COLS + c] = vga_entry(' ', vga_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

static void console_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        if (++cursor_row >= VGA_ROWS) cursor_row = 0; /* simple wrap */
        return;
    }

    VGA[cursor_row * VGA_COLS + cursor_col] = vga_entry(c, vga_color);
    if (++cursor_col >= VGA_COLS) {
        cursor_col = 0;
        if (++cursor_row >= VGA_ROWS) cursor_row = 0;
    }
}

void console_write(const char* s) {
    for (; *s; ++s) {
        console_putc(*s);
    }
}

/* ---------------- Serial (COM1) for easier debugging via -serial stdio ---------------- */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_init(void) {
    const uint16_t COM1 = 0x3F8;
    outb(COM1 + 1, 0x00);    // Disable all interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB
    outb(COM1 + 0, 0x01);    // Divisor low (115200 / 115200 = 1 => 115200 baud)
    outb(COM1 + 1, 0x00);    // Divisor high
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int serial_can_tx(void) {
    const uint16_t COM1 = 0x3F8;
    return (inb(COM1 + 5) & 0x20) != 0; // Transmitter Holding Register Empty
}

static void serial_putc(char c) {
    const uint16_t COM1 = 0x3F8;
    while (!serial_can_tx()) { }
    outb(COM1 + 0, (uint8_t)c);
}

static void serial_write(const char* s) {
    for (; *s; ++s) serial_putc(*s);
}

/* Pass Multiboot context from boot.S */
extern unsigned int mb_magic32;
extern unsigned int mb_info32;

void kernel_main(void) {
    serial_init();
    // Emit a raw byte early to confirm entry
    outb(0x3F8, 'K');
    serial_write("[kernel] entered long mode\n");
    console_clear();
    console_write("Hello, Kernel\n");
    serial_write("[kernel] printed to VGA\n");
    /* Week 2: initialize memory subsystem (parse Multiboot if available) */
    memory_init((uint64_t)mb_magic32, (uint64_t)mb_info32);
    uint64_t f1 = pmm_alloc_2m();
    uint64_t f2 = pmm_alloc_2m();
    if (f1) serial_write("[pmm] allocated 2MiB frame 1\n");
    if (f2) serial_write("[pmm] allocated 2MiB frame 2\n");
    serial_write("[pmm] init complete\n");

    /* Week 4: initialize IDT, PIC, PIT and enable interrupts */
    interrupts_init();
    pit_init(100);
    keyboard_init();
    serial_input_init(); // Initialize serial input (COM1)
    __asm__ __volatile__("sti");

    /* Week 5: set up GDT with user segments and TSS, syscalls */
    extern void gdt_init(void);
    gdt_init();
    syscall_init();
    /* Allow user-mode access to lower 64 MiB (educational simplification) */
    memory_mark_user_range(0, 64ULL * 1024 * 1024);
    /* Week 6: IPC demo — user-space client/server using syscalls */
    ipc_init();
    scheduler_init();
    /* Prepare user stacks (2 MiB each) */
    {
        uint64_t s = pmm_alloc_2m();
        if (s) ustack_server_top = s + (2ULL * 1024 * 1024);
        uint64_t c = pmm_alloc_2m();
        if (c) ustack_client_top = c + (2ULL * 1024 * 1024);
    }
    (void)task_create(start_user_server, NULL, 16384);
    (void)task_create(start_user_shell, NULL, 16384);
    scheduler_start();

    // Should not return; fallback halt
    for (;;) { __asm__ __volatile__("hlt"); }
}

/* ---- Week 3/4 demo task functions removed in favor of user-space demo ---- */
