typedef enum {
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_EXITED
} task_state_t;

typedef void (*task_fn)(void*);

void sched_maybe_yield(void);
void sched_remove_exited(void);