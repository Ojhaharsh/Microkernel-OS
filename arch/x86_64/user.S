// Enter user mode at RIP=RDI, RSP=RSI via iretq with user segments

.text
.global enter_user_mode
.type enter_user_mode, @function

/* For now, enter at CPL0 to demo syscalls reliably */
.equ USER_CS, 0x08
.equ USER_DS, 0x10

enter_user_mode:
    /* Args: RDI = user RIP, RSI = user RSP */
    mov %rsi, %rax         /* stash user RSP in rax */
    /* Debug marker before iret to user */
    mov $0x3F8, %dx
    mov $'u', %al
    out %al, (%dx)
    /* Minimal iret frame for CPL0: RFLAGS, CS, RIP */
    pushfq
    pop %rcx
    and $~0x200, %rcx
    push %rcx
    pushq $USER_CS
    push %rdi
    iretq

.size enter_user_mode, .-enter_user_mode
