// Enter user mode using SYSRET: RCX=user RIP, RSP=user RSP, R11=RFLAGS

.text
.global enter_user_mode_syscall
.type enter_user_mode_syscall, @function

enter_user_mode_syscall:
    /* Args: RDI = user RIP, RSI = user RSP */
    /* Prepare RCX (RIP) and RSP for SYSRET */
    mov %rdi, %rcx
    mov %rsi, %rsp
    /* RFLAGS: start with IF=0 (can enable later) */
    pushfq
    pop %r11
    and $~0x200, %r11
    /* Return to user mode */
    sysretq

.size enter_user_mode_syscall, .-enter_user_mode_syscall
