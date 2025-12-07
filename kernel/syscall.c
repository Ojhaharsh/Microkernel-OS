#include <stdint.h>
#include "syscall.h"
#include "ipc.h"
#include "scheduler.h"  // For task_fn and task_create

// Extern declarations for launcher functions
extern void start_user_server(void* arg);
extern void start_user_client(void* arg);
extern void start_keyboard_test(void* arg);
extern void start_user_shell(void* arg);

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);
    __asm__ __volatile__("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi; uint64_t v;
    __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    v = ((uint64_t)hi << 32) | lo; return v;
}

void syscall_init(void) {
    /* MSRs */
    const uint32_t IA32_EFER  = 0xC0000080;
    const uint32_t IA32_STAR  = 0xC0000081;
    const uint32_t IA32_LSTAR = 0xC0000082;
    const uint32_t IA32_FMASK = 0xC0000084;

    /* Enable SYSCALL/SYSRET */
    uint64_t efer = rdmsr(IA32_EFER);
    efer |= 1; /* SCE */
    wrmsr(IA32_EFER, efer);

    /* STAR: kernel CS=0x08, user CS=0x1B */
    uint64_t star = ((uint64_t)0x1B << 48) | ((uint64_t)0x08 << 32);
    wrmsr(IA32_STAR, star);

    /* LSTAR: entry point */
    extern void syscall_entry(void);
    wrmsr(IA32_LSTAR, (uint64_t)(uintptr_t)&syscall_entry);

    /* FMASK: clear IF (bit 9) during syscall; also clear TF (bit 8) */
    uint64_t fmask = (1ULL << 9) | (1ULL << 8);
    wrmsr(IA32_FMASK, fmask);
}

/* Minimal serial helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret; __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static int serial_can_tx(void) { return (inb(0x3F8 + 5) & 0x20) != 0; }
static void serial_putc(char c) { while (!serial_can_tx()) {} outb(0x3F8, (uint8_t)c); }

static uint64_t sys_write(const char* buf, uint64_t len) {
    extern void console_write(const char* s);
    console_write(buf);  // Also write to VGA console
    for (uint64_t i = 0; i < len; ++i) {
        char c = buf[i];
        if (c == '\0') break;
        serial_putc(c);
    }
    return len;
}

static uint64_t sys_yield(void) {
    extern void yield(void);
    yield();
    return 0;
}

static uint64_t sys_exit(void) {
    extern void sched_exit_current(void);
    sched_exit_current();
    return 0;
}

typedef struct {
    uint64_t dst;
    uint64_t len;
    char     data[IPC_MSG_MAX];
} ipc_packet_t;

static uint64_t sys_send(const void* upacket) {
    if (!upacket) return (uint64_t)-1;
    const ipc_packet_t* p = (const ipc_packet_t*)upacket;
    size_t n = (size_t)p->len;
    if (n > IPC_MSG_MAX) n = IPC_MSG_MAX;
    int rc = ipc_send((int)p->dst, p->data, n);
    return (uint64_t)(int64_t)rc;
}

static uint64_t sys_recv(char* buf, uint64_t maxlen) {
    int from = -1; size_t out_len = 0;
    int rc = ipc_recv(&from, buf, (size_t)maxlen, &out_len);
    if (rc < 0) return (uint64_t)-1;
    // Pack (from << 32) | len
    return ((uint64_t)(uint32_t)from << 32) | (uint64_t)(uint32_t)out_len;
}

static uint64_t sys_getchar(void) {
    extern char getchar(void);  // Use unified input interface
    char c = getchar();
    return (uint64_t)(uint8_t)c;
}

// Program registry for sys_exec
typedef struct {
    const char* name;
    void (*launcher)(void*);  // Function pointer to launcher
} user_program_t;

static const user_program_t user_programs[] = {
    {"server", (void (*)(void*))&start_user_server},
    {"client", (void (*)(void*))&start_user_client},
    {"keyboard_test", (void (*)(void*))&start_keyboard_test},
    {"shell", (void (*)(void*))&start_user_shell},
    {NULL, NULL}  // Sentinel
};

static uint64_t sys_exec(const char* prog_name) {
    if (!prog_name) return (uint64_t)-1;
    
    // Find the program in the registry
    for (const user_program_t* prog = user_programs; prog->name != NULL; ++prog) {
        // Simple string comparison (null-terminated)
        const char* a = prog->name;
        const char* b = prog_name;
        while (*a && *b && *a == *b) { ++a; ++b; }
        if (*a == '\0' && *b == '\0') {
            // Found the program, create a task for it
            extern int task_create(task_fn fn, void* arg, size_t stack_size);
            int task_id = task_create(prog->launcher, NULL, 16384);
            return (task_id >= 0) ? (uint64_t)task_id : (uint64_t)-1;
        }
    }
    
    return (uint64_t)-1;  // Program not found
}

uint64_t syscall_dispatch(uint64_t num, uint64_t arg0, uint64_t arg1) {
    switch (num) {
        case SYS_write: return sys_write((const char*)arg0, arg1);
        case SYS_yield: return sys_yield();
        case SYS_exit:  return sys_exit();
        case SYS_send:  return sys_send((const void*)arg0);
        case SYS_recv:  return sys_recv((char*)arg0, arg1);
        case SYS_getchar: return sys_getchar();
        case SYS_exec:  return sys_exec((const char*)arg0);
        default: return (uint64_t)-1;
    }
}
