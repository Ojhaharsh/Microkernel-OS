# 🏗️ System Architecture

## Kernel Components
```
kernel/
├── main.c          - Kernel entry point and initialization
├── scheduler.c     - Cooperative task scheduler
├── memory.c        - Physical memory manager
├── syscall.c       - System call dispatcher
├── ipc.c           - Mailbox-based messaging
├── interrupts.c    - IDT and interrupt handlers
├── keyboard.c      - PS/2 keyboard driver
├── serial.c        - Serial I/O functions
└── panic.c         - Kernel panic handler

arch/x86_64/
├── boot.S          - Multiboot2 header and bootstrap
├── gdt.S           - GDT and TSS setup
├── context_switch.S - Task context switching
├── syscall_entry.S - System call entry point
├── interrupt.S     - Interrupt stub handlers
├── user_*.S        - User-space programs
└── userprog.S      - User-mode entry helpers

include/
├── scheduler.h     - Task management API
├── memory.h        - Memory allocator API
├── syscall.h       - Syscall numbers and interface
├── ipc.h           - IPC function declarations
├── interrupts.h    - Interrupt setup API
└── keyboard.h      - Keyboard driver API
```

## Memory Map
```
0x0000000000000000 - 0x0000000004000000  : Identity-mapped (64 MiB)
0x0000000000100000 - kernel .text/.data  : Kernel code and data
0x00000000000B8000                       : VGA text buffer
Higher half                              : User stacks and heaps
```

## Implementation Highlights 🎯

### Task Scheduler
- Cooperative multitasking with explicit yield points
- Task creation with dynamic stack allocation
- Blocking/unblocking for synchronization
- Preference hints for responsive IPC

### System Call Mechanism
- Uses x86_64 SYSCALL instruction for fast transitions
- Preserves user context across kernel entry
- Per-task kernel stacks for isolation
- Return values in RAX register

### IPC Design
- Single-slot mailbox per task
- Synchronous send/receive primitives
- Sender blocks if receiver mailbox full
- Receiver blocks if no message available
- Automatic task wakeup on message delivery

## Technical Details ⚙️

### Compiler Flags
```
-ffreestanding -O2 -Wall -Wextra -m64
-fno-stack-protector -fno-pic -fno-pie
-mno-red-zone -mno-mmx -mno-sse -mno-avx
-mno-80387 -msoft-float
```

### Syscall Convention
- RAX: syscall number
- RDI: arg0
- RSI: arg1
- Return value in RAX

### Task Context
```c
struct context_t {
    uint64_t r15, r14, r13, r12, rbx, rbp, rsp;
    void* rip;
    uint64_t rdi, rsi;
    uint64_t first;  // First-time flag
};
```
