// Unified input test (serial + PS/2)
.text
.global user_keyboard_test

// Syscall numbers: 1=write, 2=yield, 3=exit, 6=getchar

user_keyboard_test:
    // Print welcome message
    mov $1, %rax                   // SYS_write
    lea welcome_msg(%rip), %rdi
    mov $welcome_end - welcome_msg, %rsi
    syscall

.Lloop:
    // Print prompt
    mov $1, %rax                   // SYS_write
    lea prompt_msg(%rip), %rdi
    mov $prompt_end - prompt_msg, %rsi
    syscall
    
    // Get a character using unified input
    mov $6, %rax                   // SYS_getchar
    syscall
    
    // Store the character
    mov %al, typed_char(%rip)
    
    // Print "You typed: "
    mov $1, %rax                   // SYS_write
    lea echo_msg(%rip), %rdi
    mov $echo_end - echo_msg, %rsi
    syscall
    
    // Print the character
    mov $1, %rax                   // SYS_write
    lea typed_char(%rip), %rdi
    mov $1, %rsi
    syscall
    
    // Print newline
    mov $1, %rax                   // SYS_write
    lea newline(%rip), %rdi
    mov $1, %rsi
    syscall
    
    // Check for 'q' to quit
    cmpb $'q', typed_char(%rip)
    je .Lexit
    
    jmp .Lloop

.Lexit:
    mov $1, %rax                   // SYS_write
    lea exit_msg(%rip), %rdi
    mov $exit_end - exit_msg, %rsi
    syscall
    
    mov $3, %rax                   // SYS_exit
    syscall

.data
welcome_msg:
    .asciz "[test] Unified keyboard test - type in terminal or QEMU window!\n"
welcome_end:

prompt_msg:
    .asciz "Type a key: "
prompt_end:

echo_msg:
    .asciz "You typed: '"
echo_end:

exit_msg:
    .asciz "Goodbye!\n"
exit_end:

newline:
    .asciz "'\n"

.bss
typed_char:
    .skip 1