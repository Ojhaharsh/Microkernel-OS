// GDT with user segments and a 64-bit TSS; load after entering long mode

.text
.global gdt_init
.type gdt_init, @function
 .global tss_set_rsp0

/* 16 KiB kernel stack used as RSP0 when switching from user->kernel */
.bss
.align 16
tss_stack:
    .skip 16384
tss_stack_top:

/* IST1 stack for critical exceptions like #DF */
.align 16
ist1_stack:
    .skip 8192
ist1_stack_top:

.data
.align 16
gdt64_ext:
    /* 0x00: null */
    .quad 0x0000000000000000
    /* 0x08: kernel code (DPL=0) */
    .quad 0x00AF9A000000FFFF
    /* 0x10: kernel data (DPL=0) */
    .quad 0x00AF92000000FFFF
    /* 0x18: user code (DPL=3) */
    .quad 0x00AFFA000000FFFF
    /* 0x20: user data (DPL=3) */
    .quad 0x00AFF2000000FFFF
    /* 0x28: TSS low dword */
    .quad 0
    /* 0x30: TSS high dword */
    .quad 0

gdt_desc_ext:
    .word (gdt_end_ext - gdt64_ext - 1)
    .quad gdt64_ext
gdt_end_ext:

/* 64-bit TSS structure (104 bytes) */
.bss
.align 16
tss64:
    .skip 104

.text
gdt_init:
    /* Fill TSS descriptor in GDT with base & limit */
    lea tss64(%rip), %rax           /* RAX = base */
    lea gdt64_ext(%rip), %rbx       /* RBX = &GDT */
    /* limit = 104-1 = 0x67 */
    mov $0x0067, %cx
    /* Low 8 bytes */
    mov %cx, 0x28(%rbx)             /* limit 15:0 */
    mov %ax, 0x2A(%rbx)             /* base 15:0 */
    shr $16, %rax
    mov %al, 0x2C(%rbx)             /* base 23:16 */
    /* type(0x9 avail TSS), S=0, DPL=0, P=1 => 10001001b = 0x89 */
    movb $0x89, 0x2D(%rbx)
    /* limit 19:16 (0) and flags (0) */
    movb $0x00, 0x2E(%rbx)
    mov %al, 0x2F(%rbx)             /* base 31:24 */
    /* High 8 bytes: base 63:32 */
    shr $8, %rax
    mov %rax, 0x30(%rbx)
    /* zero last 4 bytes already by .quad init */

    /* Initialize TSS.RSP0 = tss_stack_top */
    lea tss_stack_top(%rip), %rax
    mov %rax, tss64+4(%rip)   /* RSP0 at offset 4 in 64-bit TSS */
    /* Set IST1 */
    lea ist1_stack_top(%rip), %rax
    mov %rax, tss64+0x20(%rip)
    /* Disable I/O bitmap by setting iopb_offset beyond TSS limit (0x68) */
    movw $0x0068, tss64+102(%rip)

    /* Load new GDT */
    lgdt gdt_desc_ext(%rip)
    /* Load TSS selector 0x28 */
    mov $0x28, %ax
    ltr %ax
    ret

.size gdt_init, .-gdt_init

// Update TSS.RSP0 at runtime (used by scheduler when switching tasks)
.text
tss_set_rsp0:
    // arg: RDI = new RSP0
    mov %rdi, tss64+4(%rip)
    ret
