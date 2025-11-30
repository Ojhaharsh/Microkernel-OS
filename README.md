# 🧠 Microkernel OS — Educational Project

A minimal, educational **Microkernel Operating System** built from scratch to understand:
- **Scheduling**
- **Memory management**
- **System calls**
- **Process abstraction**
- **Inter-process communication (IPC)**

The project will run on **x86_64 architecture**, tested using **QEMU**, and will be written in **C + a bit of Assembly**.

---

## 🧩 Overview

This project follows a **6-week structured roadmap**, where each week focuses on a key subsystem of the operating system.

It’s heavily inspired by:
- [xv6 (MIT)](https://github.com/mit-pdos/xv6-public)
- [OSDev Wiki](https://wiki.osdev.org/)
- [Minix and L4 Microkernels](https://wiki.osdev.org/Microkernel)

Goal: By the end of 6 weeks, you’ll have a functioning **microkernel** capable of running basic user programs, switching processes, handling syscalls, and performing message-based IPC.

---

## ⚙️ Tools & Environment Setup

You’ll need:

- `gcc` or a **cross-compiler** (`x86_64-elf-gcc`)
- `make`
- `nasm`
- `qemu-system-x86_64`
- `grub-pc-bin` or `grub2`
- `gdb` (for kernel debugging)
- A Linux environment (native or via WSL)

**Optional:**  
Use Docker or a dev container with preinstalled build tools to keep the environment clean.

---

## 📆 6-Week Plan

### 🗓️ Week 1 — Bootloader & Kernel Entry
**Goal:** Boot into 64-bit mode and print “Hello, Kernel” on QEMU.

**Tasks:**
1. Setup project structure and Makefile.
2. Write a GRUB-compatible multiboot header.
3. Implement a 64-bit entry point in assembly (`boot.S`).
4. Switch to C code with a `kernel_main()` function.
5. Implement VGA or serial console output for debugging.
6. Verify boot via QEMU:  
   ```bash
   qemu-system-x86_64 -kernel build/kernel.elf -serial stdio
Milestone:
✅ Kernel boots and prints “Hello Kernel” in QEMU.

🗓️ Week 2 — Memory Management (Paging + Allocator)
Goal: Create basic paging, identity map memory, and implement a physical memory allocator.

Tasks:

Implement identity-mapped paging (or high-half if preferred).

Build a page frame allocator (bitmap or linked list).

Write a simple kmalloc() for dynamic kernel allocations.

Understand page tables: PML4 → PDPT → PD → PT.

Test by allocating and printing physical pages.

Milestone:
✅ Kernel successfully maps memory, allocates/free pages, and maintains memory tables.

🗓️ Week 3 — Tasks & Context Switching
Goal: Introduce multitasking (kernel threads) and basic context switching.

Tasks:

Define a task_struct with register states, stack, and state flags.

Write assembly code for saving/restoring registers (context switch).

Implement create_task() to spawn new kernel tasks.

Switch between two kernel threads manually.

Test by printing from two tasks alternately.

Milestone:
✅ Two kernel tasks print messages alternately via manual scheduler calls.

🗓️ Week 4 — Scheduler & Timer Interrupts
Goal: Automate context switching using a timer interrupt.

Tasks:

Setup the Programmable Interval Timer (PIT) or APIC timer.

Create a scheduler (round-robin or cooperative).

Implement scheduler_tick() and schedule_next().

Switch tasks automatically on each timer tick.

Introduce process states: READY, RUNNING, BLOCKED.

Milestone:
✅ Preemptive multitasking achieved; tasks run automatically at time slices.

🗓️ Week 5 — Syscalls & User Mode
Goal: Create a syscall interface and run a user program.

Tasks:

Implement syscall mechanism (int 0x80 or syscall instruction).

Setup user/kernel privilege levels using segments (GDT/TSS).

Write syscall handler (syscall_dispatch()).

Implement basic syscalls: write(), yield(), exit().

Build a minimal ELF loader to load a user program.

Create a test user program that prints using write() syscall.

Milestone:
✅ Kernel runs a user process that prints via a syscall.

Notes:
- Uses x86_64 SYSCALL/SYSRET (not int 0x80) via MSRs: `EFER.SCE`, `STAR`, `LSTAR`, `FMASK`.
- Entry stub saves RCX/R11, switches to a dedicated kernel stack, calls C dispatcher, returns with `SYSRETQ`.
- Minimal syscalls: `write`, `yield`, `exit` (serial-only I/O for visibility).

🗓️ Week 6 — IPC & Refinement
Goal: Implement inter-process communication and refine kernel components.

Tasks:

Implement a simple message-passing system (send(), recv()).

Add blocking/wakeup mechanisms for tasks waiting on messages.

Clean up scheduler and memory management code.

Add error handling, logging, and a panic system.

Write documentation for the kernel’s architecture.

Optional: Add basic drivers (keyboard, framebuffer).

Milestone:
✅ Two tasks exchange messages through kernel-managed IPC (blocking send/recv).
Kernel stable and modular.

Design:
- Per-task single-slot mailbox (bounded, fixed-size messages up to `IPC_MSG_MAX`).
- Blocking semantics: `ipc_send()` waits until receiver mailbox is empty; `ipc_recv()` waits until a message arrives.
- Wakeup path: `sched_unblock(dst)` wakes blocked receiver; sender yields to encourage handoff.
- Critical sections around mailbox fill/empty guarded by `cli/sti` to avoid timer ISR preemption mid-update.
- Scheduler helpers added: `sched_current_id()`, `sched_block_current()`, `sched_unblock(id)`.

Demo:
- Two kernel tasks (`ping`, `pong`) exchange a message; serial output shows the flow.
- Expected serial log snippet:
   - `[ping] start`
   - `[pong] start`
   - `[pong] got hi`
   - `[ping] sent`

Files:
- `kernel/ipc.c`, `include/ipc.h` — mailbox IPC.
- `kernel/scheduler.c`, `arch/x86_64/context_switch.S` — blocking/wakeup + corrected context switch (only sets args on first entry; fixes resume corruption).
- `kernel/syscall.c`, `arch/x86_64/syscall_entry.S` — syscall infra (Week 5).
- `kernel/main.c` — initializes interrupts, IPC, and starts ping/pong tasks.


🧠 Learning References
xv6 source + commentary

OSDev Wiki — Paging, Interrupts, Syscalls

Operating Systems: Three Easy Pieces

L4 Microkernel Papers

---

## ▶️ Run Instructions

Build and run from Windows (WSL required):

```bat
wsl bash -lc "cd '/mnt/c/Users/KIIT/Desktop/Microkernel OS' && make -j && QEMU_AUDIO_DRV=none make run"
```

Headless (no GUI window, serial only):

```bat
wsl bash -lc "cd '/mnt/c/Users/KIIT/Desktop/Microkernel OS' && make -j && QEMU_AUDIO_DRV=none qemu-system-x86_64 -cdrom build/microkernel.iso -serial stdio -display none"
```

Notes:
- Output is via serial (COM1), visible on your terminal. VGA remains minimal.
- If you see GTK warnings from QEMU, they’re harmless; use headless mode to suppress.

