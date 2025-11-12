#ifndef TASK_H
#define TASK_H

// Placeholder types and prototypes for Week 3 tasks/threads
typedef struct task_struct {
    // registers, stack pointer, state, etc.
    void* stack_ptr;
    int state;
} task_t;

#endif // TASK_H
