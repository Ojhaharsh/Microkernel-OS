#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Syscall numbers
enum {
    SYS_write = 1,
    SYS_yield = 2,
    SYS_exit  = 3,
    SYS_send  = 4,
    SYS_recv  = 5,
    SYS_getchar = 6,
    SYS_exec = 7,  // Launch user program by name
};

// Kernel-side dispatcher; called from syscall_entry
uint64_t syscall_dispatch(uint64_t num, uint64_t arg0, uint64_t arg1);

// Initializes SYSCALL/SYSRET MSRs
void syscall_init(void);

#endif // SYSCALL_H
