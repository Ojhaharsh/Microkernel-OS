// x86_64 SYSCALL/SYSRET entry using a dedicated kernel stack

.text
.global syscall_entry
.type syscall_entry, @function

.extern g_current_kstack_top

.text
syscall_entry:
    /* Save user RCX (RIP) and R11 (RFLAGS) */
    push %rcx
    push %r11
    /* Save user RSP and switch to current task's kernel stack */
    mov %rsp, %r10
    lea g_current_kstack_top(%rip), %rsp
    mov (%rsp), %rsp
    /* Save user RSP on kernel stack */
    push %r10
    /* Save callee-saved we might clobber */
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    /* Arrange C args: rdi=num, rsi=a0, rdx=a1
       On entry: rax=num, rdi=a0, rsi=a1 */
    mov %rdi, %r8    /* save a0 */
    mov %rsi, %r9    /* save a1 */
    mov %rax, %rdi   /* num */
    mov %r8,  %rsi   /* a0 */
    mov %r9,  %rdx   /* a1 */
    call syscall_dispatch
    /* Restore callee-saved */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    /* Restore user RSP */
    pop %r10
    mov %r10, %rsp
    /* Restore user RFLAGS and RIP into R11 and RCX */
    pop %r11
    pop %rcx
    /* Return to user */
    sysretq

.size syscall_entry, .-syscall_entry
