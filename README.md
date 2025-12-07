# Microkernel OS - Educational x86_64 Kernel

A minimal **Microkernel Operating System** built from scratch for x86_64 architecture. This project demonstrates core OS concepts including cooperative multitasking, memory management, system calls, and inter-process communication.

> **Documentation Note 📚**
>
> *   **Architecture 🏗️** and **Technical Details ⚙️**: See [**docs/ARCHITECTURE.md**](docs/ARCHITECTURE.md)
> *   **Building and Running 🚀🏃🏼‍➡️** and **Usage 📝**: See [**docs/USAGE.md**](docs/USAGE.md)

## Features 👇🏼

<details>
<summary><strong>Core Kernel</strong></summary>

- **x86_64 Long Mode** - Full 64-bit architecture support
- **GRUB Multiboot2** - Standard bootloader compatibility
- **Identity Paging** - Simple memory mapping for lower 64 MiB
- **Physical Memory Manager** - 2 MiB frame allocator
- **VGA Text Console** - 80x25 display with color support
- **Serial Output** - COM1 debugging interface (115200 baud)
</details>

<details>
<summary><strong>Process Management</strong></summary>

- **Cooperative Scheduler** - Round-robin task switching
- **Task States** - UNUSED, READY, RUNNING, BLOCKED, EXITED
- **Context Switching** - Full register preservation
- **Per-Task Stacks** - Isolated kernel and user stacks
- **Dynamic Task Creation** - Runtime program launching
</details>

<details>
<summary><strong>System Calls</strong></summary>

- **SYSCALL/SYSRET** - Fast x86_64 syscall mechanism
- **write(1)** - Console and serial output
- **yield(2)** - Cooperative task switching
- **exit(3)** - Task termination
- **send(4)** - IPC message sending
- **recv(5)** - IPC message receiving
- **getchar(6)** - Unified keyboard/serial input
- **exec(7)** - Dynamic program execution
</details>

<details>
<summary><strong>Inter-Process Communication</strong></summary>

- **Mailbox IPC** - Per-task single-slot messaging
- **Blocking Semantics** - Tasks block until messages arrive
- **Server/Client Demo** - Ping-pong message exchange
</details>

<details>
<summary><strong>Input/Output</strong></summary>

- **PS/2 Keyboard** - Scancode translation with shift support
- **Serial Input** - COM1 character reception via IRQ4
- **Unified Input** - Single getchar() interface for both sources
- **PIC & PIT** - Interrupt management and timer
</details>

<details>
<summary><strong>User Programs</strong></summary>

- **IPC Server** - Receives ping messages, responds with pong
- **IPC Client** - Sends 5 ping messages, waits for pong replies
- **Interactive Shell** - Command-line interface with program launching
- **Keyboard Test** - Interactive input testing program
</details>

## Roadmap 🗺️

We are actively working on the following features. Contributions are welcome!

<details>
<summary><strong>Phase 1: Memory Management (Next Up)</strong></summary>

- [ ] **User-Space Allocator**: Implement `malloc` and `free` in `user/lib/malloc.c`.
- [ ] **Heap Management**: Manage a static heap or request pages from the kernel.
</details>

<details>
<summary><strong>Phase 2: Storage & VFS (Planned)</strong></summary>

- [ ] **Virtual File System**: Create a VFS layer with `open`, `read`, `close`.
- [ ] **InitRD**: Parse Multiboot2 modules to load a TAR RAM disk.
- [ ] **File Support**: Allow user programs to read text files and scripts.
</details>

<details>
<summary><strong>Phase 3: Graphics (Planned)</strong></summary>

- [ ] **Framebuffer**: Switch to VESA/VGA graphics mode.
- [ ] **Graphics Driver**: Implement `put_pixel` and basic shape drawing.
- [ ] **GUI**: Build a simple windowing system.
</details>

## Implementation Highlights 🎯

<details>
<summary><strong>Task Scheduler</strong></summary>

- Cooperative multitasking with explicit yield points
- Task creation with dynamic stack allocation
- Blocking/unblocking for synchronization
- Preference hints for responsive IPC
</details>

<details>
<summary><strong>System Call Mechanism</strong></summary>

- Uses x86_64 SYSCALL instruction for fast transitions
- Preserves user context across kernel entry
- Per-task kernel stacks for isolation
- Return values in RAX register
</details>

<details>
<summary><strong>IPC Design</strong></summary>

- Single-slot mailbox per task
- Synchronous send/receive primitives
- Sender blocks if receiver mailbox full
- Receiver blocks if no message available
- Automatic task wakeup on message delivery
</details>

## Educational Value 📚

This project demonstrates:
1. **Bare-metal programming** - No standard library, direct hardware access
2. **x86_64 architecture** - Long mode, paging, syscalls, interrupts
3. **OS design patterns** - Scheduler, memory manager, device drivers
4. **Synchronization** - Blocking IPC, task states, critical sections
5. **User/kernel separation** - Privilege levels, stack switching, syscalls

## Known Limitations 🐛

- No process isolation (all tasks share address space)
- No dynamic memory allocator (kmalloc only)
- Fixed maximum of 8 tasks
- No filesystem or persistent storage
- Single-slot IPC (no message queues)
- Cooperative scheduling only (no preemption)

## Learning Resources 📖

This project covers concepts from:
- Intel® 64 and IA-32 Architectures Software Developer's Manual
- OSDev Wiki (wiki.osdev.org)
- "Operating Systems: Three Easy Pieces" by Remzi H. Arpaci-Dusseau

## License 📄

This is an educational project. Feel free to use, modify, and learn from it.

## Acknowledgments 🙏

Built as a learning exercise to understand operating system internals from the ground up. Special thanks to the OSDev community for extensive documentation and support.

---

**Status**: ✅ Complete - All core features implemented and tested
**Last Updated**: December 2025

```

Notes:
- Output is via serial (COM1), visible on your terminal. VGA remains minimal.
- If you see GTK warnings from QEMU, they’re harmless; use headless mode to suppress.

