// Simple shell for manual program execution
.text
.global user_shell_start

// Syscall numbers: 1=write, 2=yield, 3=exit, 6=getchar, 7=exec

user_shell_start:
    // Print welcome message
    mov $1, %rax                   // SYS_write
    lea welcome_msg(%rip), %rdi
    mov $welcome_end - welcome_msg, %rsi
    syscall

.Lshell_loop:
    // Print prompt
    mov $1, %rax                   // SYS_write
    lea prompt_msg(%rip), %rdi
    mov $prompt_end - prompt_msg, %rsi
    syscall

    // Read command line
    lea cmd_buffer(%rip), %rbx     // rbx = buffer pointer
    xor %rcx, %rcx                 // rcx = buffer index

.Lread_loop:
    // Get a character
    mov $6, %rax                   // SYS_getchar
    syscall
    mov %al, (%rbx,%rcx)           // Store character in buffer

    // Echo the character
    mov $1, %rax                   // SYS_write
    mov %rbx, %rdi
    add %rcx, %rdi                 // Point to current character
    mov $1, %rsi
    syscall

    // Check for newline (enter) - check both \n and \r
    cmpb $'\n', (%rbx,%rcx)
    je .Lprocess_cmd
    cmpb $'\r', (%rbx,%rcx)
    je .Lprocess_cmd

    // Check for backspace
    cmpb $'\b', (%rbx,%rcx)
    je .Lbackspace

    // Check buffer overflow
    inc %rcx
    cmp $63, %rcx
    jge .Lread_loop                // Ignore if buffer full

    jmp .Lread_loop

.Lbackspace:
    // Handle backspace
    test %rcx, %rcx
    jz .Lread_loop                 // Nothing to backspace
    dec %rcx
    jmp .Lread_loop

.Lprocess_cmd:
    // Null-terminate the command (replace newline with null)
    movb $0, (%rbx,%rcx)

    // Skip leading whitespace
    lea cmd_buffer(%rip), %rbx
    mov %rbx, %rdx                 // rdx = start of command

.Lskip_whitespace:
    movb (%rdx), %al
    test %al, %al
    jz .Lempty_cmd
    cmpb $' ', %al
    je .Lnext_char
    cmpb $'\t', %al
    je .Lnext_char
    cmpb $'\n', %al
    je .Lnext_char
    cmpb $'\r', %al
    je .Lnext_char
    // Found non-whitespace, process command
    jmp .Lfound_cmd

.Lnext_char:
    inc %rdx
    jmp .Lskip_whitespace



.Lfound_cmd:
    // Check for built-in commands
    // "exit" command
    lea exit_cmd(%rip), %rsi
    mov %rdx, %rdi
    call strcmp
    test %eax, %eax
    jz .Lexit_shell

    // "help" command
    lea help_cmd(%rip), %rsi
    mov %rdx, %rdi
    call strcmp
    test %eax, %eax
    jz .Lshow_help

    // Try to execute as program name
    mov $7, %rax                   // SYS_exec
    mov %rdx, %rdi                 // program name
    syscall

    // Check result
    cmp $-1, %rax
    je .Lcmd_not_found

    // Success - print message
    mov $1, %rax                   // SYS_write
    lea launched_msg(%rip), %rdi
    mov $launched_end - launched_msg, %rsi
    syscall
    jmp .Lshell_loop


.Lcmd_not_found:
    mov $1, %rax                   // SYS_write
    lea not_found_msg(%rip), %rdi
    mov $not_found_end - not_found_msg, %rsi
    syscall
    jmp .Lshell_loop

.Lshow_help:
    mov $1, %rax                   // SYS_write
    lea help_msg(%rip), %rdi
    mov $help_end - help_msg, %rsi
    syscall
    jmp .Lshell_loop

.Lempty_cmd:
    jmp .Lshell_loop

.Lexit_shell:
    mov $1, %rax                   // SYS_write
    lea goodbye_msg(%rip), %rdi
    mov $goodbye_end - goodbye_msg, %rsi
    syscall
    mov $3, %rax                   // SYS_exit
    syscall

// Simple string compare function (rdi=str1, rsi=str2)
strcmp:
    xor %eax, %eax
.Lstrcmp_loop:
    movb (%rdi), %al
    movb (%rsi), %cl
    cmp %al, %cl
    jne .Lstrcmp_done
    test %al, %al
    jz .Lstrcmp_done
    inc %rdi
    inc %rsi
    jmp .Lstrcmp_loop
.Lstrcmp_done:
    movsx %al, %eax
    movsx %cl, %ecx
    sub %ecx, %eax
    ret

.data
welcome_msg:
    .ascii "Microkernel Shell v1.0\n"
    .ascii "Type 'help' for commands, 'exit' to quit\n\n"
welcome_end:

prompt_msg:
    .ascii "shell> "
prompt_end:

help_msg:
    .ascii "Available commands:\n"
    .ascii "  server        - Start IPC server\n"
    .ascii "  client        - Start IPC client\n"
    .ascii "  keyboard_test - Test keyboard input\n"
    .ascii "  help          - Show this help\n"
    .ascii "  exit          - Exit shell\n\n"
help_end:

launched_msg:
    .ascii "Program launched\n"
launched_end:

not_found_msg:
    .ascii "Command not found\n"
not_found_end:

goodbye_msg:
    .ascii "Goodbye!\n"
goodbye_end:

exit_cmd: .asciz "exit"
help_cmd: .asciz "help"

.bss
cmd_buffer: .space 64