// Simple keyboard monitor test
.text
.global user_keyboard_test

// Syscall numbers: 1=write, 2=yield, 3=exit

user_keyboard_test:
    // Print welcome message
    mov $1, %rax                   // SYS_write
    lea welcome_msg(%rip), %rdi
    mov $welcome_end - welcome_msg, %rsi
    syscall

    // Loop and yield - let keyboard interrupts show
    mov $0, %rbx                   // counter
.Lloop:
    inc %rbx
    
    // Print status every 64 iterations
    mov %rbx, %rax
    and $63, %rax                  // Every 64 iterations
    cmp $0, %rax
    jne .Lskip_print
    
    mov $1, %rax                   // SYS_write
    lea status_msg(%rip), %rdi
    mov $status_end - status_msg, %rsi
    syscall
    
.Lskip_print:
    // Yield to let interrupts happen
    mov $2, %rax                   // SYS_yield
    syscall
    
    // Exit after many iterations
    cmp $1024, %rbx
    jl .Lloop
    
    mov $1, %rax                   // SYS_write
    lea exit_msg(%rip), %rdi
    mov $exit_end - exit_msg, %rsi
    syscall
    
    mov $3, %rax                   // SYS_exit
    syscall

.data
welcome_msg:
    .asciz "[test] Keyboard monitor - try typing in QEMU window\n"
welcome_end:

status_msg:
    .asciz "[test] Running... try pressing keys\n"
status_end:

exit_msg:
    .asciz "[test] Test completed\n"
exit_end: