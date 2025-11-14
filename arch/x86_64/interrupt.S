// Interrupt stubs for x86_64 (IRQ0 timer)

.text
.global isr_irq0
.type isr_irq0, @function
isr_irq0:
    // Save registers (a minimal set; SysV caller-saved + callee-saved we use)
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbx
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    // Call C handler
    call timer_isr

    // Send EOI to PIC master
    mov $0x20, %al
    out %al, $0x20

    // Restore regs
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rbx
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq

// IRQ1 keyboard interrupt handler
.text
.global isr_irq1
.type isr_irq1, @function
isr_irq1:
    // Save registers
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbx
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    // Call keyboard handler
    call keyboard_irq

    // Send EOI to PIC master
    mov $0x20, %al
    out %al, $0x20

    // Restore registers
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rbx
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq

// Syscall interrupt handler (int 0x80), DPL=3 gate
.text
.global isr_syscall
.type isr_syscall, @function
isr_syscall:
    /* Debug: print 'S' to serial */
    mov $0x3F8, %dx
    mov $'S', %al
    out %al, (%dx)
    /* Minimal save of callee-saved regs for the C call */
    push %rbp
    mov %rsp, %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    /* Arrange args for C: rdi=num (already in rax), rsi=a0 (buf), rdx=a1 (len) */
    mov %rax, %rdi
    /* rdi and rsi already hold a0/a1 from user as per SysV calling convention */
    call syscall_dispatch
    /* Restore */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    iretq

// Simple #GP (13) and #PF (14) exception handlers to aid debugging
.text
.global isr_gp
.type isr_gp, @function
isr_gp:
    /* On CPU entry: error code pushed by hardware */
    /* Print 'G' */
    mov $0x3F8, %dx
    mov $'G', %al
    out %al, (%dx)
    hlt
    jmp .

.global isr_pf
.type isr_pf, @function
isr_pf:
    mov $0x3F8, %dx
    mov $'F', %al
    out %al, (%dx)
    hlt
    jmp .

.global isr_ts
.type isr_ts, @function
isr_ts:
    mov $0x3F8, %dx
    mov $'T', %al
    out %al, (%dx)
    hlt
    jmp .

.global isr_ss
.type isr_ss, @function
isr_ss:
    mov $0x3F8, %dx
    mov $'S', %al
    out %al, (%dx)
    hlt
    jmp .

.global isr_np
.type isr_np, @function
isr_np:
    mov $0x3F8, %dx
    mov $'N', %al
    out %al, (%dx)
    hlt
    jmp .

.global isr_df
.type isr_df, @function
isr_df:
    mov $0x3F8, %dx
    mov $'D', %al
    out %al, (%dx)
    hlt
    jmp .

// Device-not-available (#NM) handler to detect SSE usage
.text
.global isr_nm
.type isr_nm, @function
isr_nm:
    mov $0x3F8, %dx
    mov $'M', %al
    out %al, (%dx)
    hlt
    jmp .

// Undefined opcode (#UD)
.text
.global isr_ud
.type isr_ud, @function
isr_ud:
    mov $0x3F8, %dx
    mov $'U', %al
    out %al, (%dx)
    hlt
    jmp .

// Divide error (#DE)
.text
.global isr_de
.type isr_de, @function
isr_de:
    mov $0x3F8, %dx
    mov $'0', %al
    out %al, (%dx)
    hlt
    jmp .

// Bound range exceeded (#BR)
.text
.global isr_br
.type isr_br, @function
isr_br:
    mov $0x3F8, %dx
    mov $'R', %al
    out %al, (%dx)
    hlt
    jmp .

// Overflow (#OF)
.text
.global isr_of
.type isr_of, @function
isr_of:
    mov $0x3F8, %dx
    mov $'O', %al
    out %al, (%dx)
    hlt
    jmp .

// Breakpoint (#BP)
.text
.global isr_bp
.type isr_bp, @function
isr_bp:
    mov $0x3F8, %dx
    mov $'B', %al
    out %al, (%dx)
    hlt
    jmp .

// Alignment check (#AC)
.text
.global isr_ac
.type isr_ac, @function
isr_ac:
    mov $0x3F8, %dx
    mov $'A', %al
    out %al, (%dx)
    hlt
    jmp .

// x87 FPU floating-point error (#MF)
.text
.global isr_mf
.type isr_mf, @function
isr_mf:
    mov $0x3F8, %dx
    mov $'X', %al
    out %al, (%dx)
    hlt
    jmp .

// Debug (#DB)
.text
.global isr_db
.type isr_db, @function
isr_db:
    mov $0x3F8, %dx
    mov $'1', %al
    out %al, (%dx)
    hlt
    jmp .

// Non-maskable interrupt (#NMI)
.text
.global isr_nmi
.type isr_nmi, @function
isr_nmi:
    mov $0x3F8, %dx
    mov $'2', %al
    out %al, (%dx)
    hlt
    jmp .

// Reserved (#15)
.text
.global isr_res15
.type isr_res15, @function
isr_res15:
    mov $0x3F8, %dx
    mov $'5', %al
    out %al, (%dx)
    hlt
    jmp .

// Generic exception handler for unknown vectors
.text
.global isr_any
.type isr_any, @function
isr_any:
    mov $0x3F8, %dx
    mov $'E', %al
    out %al, (%dx)
    hlt
    jmp .
