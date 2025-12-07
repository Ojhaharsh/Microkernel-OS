#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

typedef void (*task_fn)(void*);

// Basic process/task states for Week 4
typedef enum {
	TASK_UNUSED = 0,
	TASK_READY = 1,
	TASK_RUNNING = 2,
	TASK_BLOCKED = 3,
	TASK_EXITED = 4,
} task_state_t;

void scheduler_init(void);
int  task_create(task_fn fn, void* arg, size_t stack_size);
void scheduler_start(void);
void yield(void);

// Called from timer ISR to signal a time slice has elapsed
void scheduler_tick(void);

// Explicitly pick and switch to the next READY task (round-robin)
void schedule_next(void);

// Hint scheduler to run a specific READY task next (best-effort)
void sched_prefer_next(int task_id);

// Cooperative yield point used in task loops to honor preemption flag
void sched_maybe_yield(void);

// Week 6: Blocking/wakeup helpers and task identity
int  sched_current_id(void);
void sched_block_current(void);
void sched_unblock(int task_id);

// Terminate the current task and switch away
void sched_exit_current(void);

// Remove exited tasks from scheduler
void sched_remove_exited(void);

#endif // SCHEDULER_H
