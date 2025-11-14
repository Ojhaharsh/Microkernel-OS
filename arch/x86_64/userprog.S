// Tiny embedded user program: writes a message via int 0x80 then loops

.text
.global userprog_start
.global userprog_end

userprog_start:
    /* rax = SYS_write(1), rdi=buf, rsi=len */
    mov $1, %rax              /* SYS_write */
    lea msg(%rip), %rdi       /* buf */
    mov $msg_end - msg, %rsi  /* len */
    syscall

    /* Exit */
    mov $3, %rax              /* SYS_exit */
    syscall

msg:
    .asciz "Hello from user via syscall\n"
msg_end:

userprog_end:
