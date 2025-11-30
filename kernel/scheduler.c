// Week 3: Minimal cooperative scheduler (kernel threads)
#include <stdint.h>
#include <stddef.h>
#include "scheduler.h"

typedef struct context_t {
	uint64_t r15, r14, r13, r12, rbx, rbp, rsp;
	void*    rip;
	uint64_t rdi, rsi;
    uint64_t first; // 0 before first entry, then set to 1
} context_t;

typedef struct task_t {
	context_t ctx;
	uint8_t*  stack_base;
	size_t    stack_size;
	uint8_t*  kstack_base;
	size_t    kstack_size;
	task_state_t state; // READY, RUNNING, BLOCKED, EXITED
} task_t;

#define MAX_TASKS 8
static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int current = -1;
static volatile int need_resched = 0;
static volatile int prefer_next = -1;

extern void context_switch(context_t* old, context_t* newc);
extern void tss_set_rsp0(uint64_t rsp0);
uint64_t g_current_kstack_top = 0;

/* Minimal serial for debug breadcrumbs */
static inline void outb(uint16_t port, uint8_t val) {
	__asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
	uint8_t ret; __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static int serial_can_tx(void) { return (inb(0x3F8 + 5) & 0x20) != 0; }
static void serial_putc(char c) { while (!serial_can_tx()) {} outb(0x3F8, (uint8_t)c); }
static void serial_write(const char* s) { for(;*s;++s) serial_putc(*s); }

static void task_trampoline(void (*fn)(void*), void* arg) {
	serial_write("[sched] trampoline enter\n");
	fn(arg);
	for(;;) { sched_maybe_yield(); }
}

void scheduler_init(void) {
	task_count = 0;
	current = -1;
	serial_write("[sched] init\n");
}

int task_create(task_fn fn, void* arg, size_t stack_size) {
	if (task_count >= MAX_TASKS) return -1;
	task_t* t = &tasks[task_count];
	t->stack_size = stack_size ? stack_size : 16384;
	extern void* kmalloc(size_t);
	t->stack_base = (uint8_t*)kmalloc(t->stack_size);
	if (!t->stack_base) return -1;
	// Allocate per-task kernel stack for syscalls/interrupts
	t->kstack_size = 16384;
	t->kstack_base = (uint8_t*)kmalloc(t->kstack_size);
	if (!t->kstack_base) return -1;
	uint8_t* sp = t->stack_base + t->stack_size;
	// Align stack to 16 bytes for SysV ABI, then make it 16n+8 for function entry via jmp
	sp = (uint8_t*)((uintptr_t)sp & ~((uintptr_t)15));
	sp -= 8; // simulate a pushed return address so entry sees RSP%16==8
	*(uint64_t*)sp = 0ULL; // dummy return address

	// Prepare fake return frame: RIP = task_trampoline, with args in RDI/RSI
	t->ctx.r15 = 0; t->ctx.r14 = 0; t->ctx.r13 = 0; t->ctx.r12 = 0; t->ctx.rbx = 0; t->ctx.rbp = 0;
	t->ctx.rip = (void*)task_trampoline;
	t->ctx.rdi = (uint64_t)(uintptr_t)fn;
	t->ctx.rsi = (uint64_t)(uintptr_t)arg;
    t->ctx.first = 0;
	// Set desired RSP
	t->ctx.rsp = (uint64_t)(uintptr_t)sp;
	t->state = TASK_READY;
	return task_count++;
}

static int pick_next(void) {
	if (task_count == 0) return -1;
	int start = (current + 1) % task_count;
	for (int i = 0; i < task_count; ++i) {
		int idx = (start + i) % task_count;
		if (tasks[idx].state != TASK_EXITED && tasks[idx].state != TASK_BLOCKED) return idx;
	}
	return -1;
}

void yield(void) {
	int next = -1;
	if (prefer_next >= 0 && prefer_next < task_count && tasks[prefer_next].state == TASK_READY) {
		next = prefer_next;
	} else {
		next = pick_next();
	}
	prefer_next = -1;
	if (next < 0 || next == current) return; // nothing to do
	int prev = current;
	current = next;
	// State bookkeeping
	if (prev >= 0 && tasks[prev].state != TASK_EXITED) tasks[prev].state = TASK_READY;
	tasks[current].state = TASK_RUNNING;
	// Update per-task kernel stack pointer and TSS.RSP0
	g_current_kstack_top = (uint64_t)(uintptr_t)(tasks[current].kstack_base + tasks[current].kstack_size);
	tss_set_rsp0(g_current_kstack_top);
	if (prev < 0) {
		// First switch from bootstrap context
		context_switch(NULL, &tasks[current].ctx);
	} else {
		context_switch(&tasks[prev].ctx, &tasks[current].ctx);
	}
}

void scheduler_start(void) {
	yield();
}

void scheduler_tick(void) { need_resched = 1; }

void schedule_next(void) {
	// For soft-preemption design: just yield to the next READY task
	yield();
}

void sched_maybe_yield(void) {
	if (need_resched) {
		need_resched = 0;
		schedule_next();
	}
}

// ---- Week 6 helpers ----
int sched_current_id(void) {
	return current;
}

void sched_block_current(void) {
	if (current < 0) return;
	tasks[current].state = TASK_BLOCKED;
	// block current and yield
	yield();
}

void sched_unblock(int task_id) {
	if (task_id < 0 || task_id >= task_count) return;
	if (tasks[task_id].state == TASK_BLOCKED) {
		tasks[task_id].state = TASK_READY;
	}
}

void sched_prefer_next(int task_id) {
	if (task_id < 0 || task_id >= task_count) return;
	prefer_next = task_id;
}

void sched_exit_current(void) {
    if (current < 0) {
        for(;;) { __asm__ __volatile__("hlt"); }
    }
    tasks[current].state = TASK_EXITED;
    sched_remove_exited(); // Remove exited tasks
    yield();
    // Should never resume here; if we do, halt.
    for(;;) { __asm__ __volatile__("hlt"); }
}

void sched_remove_exited(void) {
    for (int i = 0; i < task_count; ++i) {
        if (tasks[i].state == TASK_EXITED) {
            tasks[i].state = TASK_UNUSED; // Mark as unused
        }
    }
}

#define TASK_UNUSED 0 // Unused task state

