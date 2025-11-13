// x86_64 context switch for cooperative scheduler (Week 3)
// Saves callee-saved regs, RSP, and the caller's return RIP from stack; restores new context and jumps.

.text
.global context_switch
.type context_switch, @function
// void context_switch(context_t* old, context_t* new)
// context_t layout (see scheduler.c): r15,r14,r13,r12,rbx,rbp,rsp,rip,rdi,rsi,first
context_switch:
    // If old != NULL, save callee-saved registers, RSP, and caller's return RIP
    test %rdi, %rdi
    jz .Lload
    mov %r15, 0(%rdi)
    mov %r14, 8(%rdi)
    mov %r13, 16(%rdi)
    mov %r12, 24(%rdi)
    mov %rbx, 32(%rdi)
    mov %rbp, 40(%rdi)
    mov %rsp, 48(%rdi)
    mov (%rsp), %rax
    mov %rax, 56(%rdi)
.Lload:
    // Load new context
    // Preserve context pointer in r10 for later stores
    mov %rsi, %r10
    mov 0(%rsi), %r15
    mov 8(%rsi), %r14
    mov 16(%rsi), %r13
    mov 24(%rsi), %r12
    mov 32(%rsi), %rbx
    mov 40(%rsi), %rbp
    mov 48(%rsi), %rsp
    // Read saved RIP early before clobbering pointer;
    mov 56(%rsi), %rax
    // Conditionally set args on first entry only
    mov 80(%rsi), %rcx        // first flag
    test %rcx, %rcx
    jne .Lskip_args
    mov 64(%rsi), %rdi        // arg0 (fn)
    mov 72(%rsi), %rdx        // arg1 (arg)
    mov %rdx, %rsi
    mov $1, %rcx
    mov %rcx, 80(%r10)        // mark first=1 using saved context pointer
.Lskip_args:
    // Resume: overwrite return address at top of stack and ret
    mov %rax, (%rsp)
    ret

.size context_switch, .-context_switch
