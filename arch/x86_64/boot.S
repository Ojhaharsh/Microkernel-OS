/*
 * Multiboot-compliant entry that switches to x86_64 long mode and calls kernel_main.
 * Week 1 goal: Boot into 64-bit mode and print "Hello, Kernel".
 */

/* Multiboot v1 header (GRUB loads us in 32-bit protected mode) */
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Multiboot2 header for ELF64-friendly boot */

.section .text.boot
.code32

/* Place multiboot headers at the very beginning */
.align 4
multiboot1_header:
.long MAGIC
.long FLAGS
.long CHECKSUM

.align 8
mb2_header:
    .long 0xE85250D6            /* magic */
    .long 0x00000000            /* architecture (i386) */
    .long mb2_header_end - mb2_header /* total header length */
    .long -(0xE85250D6 + 0x00000000 + (mb2_header_end - mb2_header)) /* checksum */

    /* End tag */
    .word 0    /* type */
    .word 0    /* flags */
    .long 8    /* size */
mb2_header_end:

.globl _start
.type _start, @function

_start:
    cli

    /* Debug: Very early output to COM1 to confirm we boot */
    mov $0x3F8, %dx
    mov $'B', %al
    out %al, %dx

    /* Save Multiboot magic (EAX) and info pointer (EBX) */
    mov %eax, mb_magic32
    mov %ebx, mb_info32

    /* Load a minimal GDT with 64-bit code and data descriptors */
    lgdt gdt_descriptor

    /* Load data segments (selector 0x10 => GDT entry 2, data) */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    /* Setup a temporary 32-bit stack for transition */
    mov $stack32_top, %esp

    /* Enable PAE */
    mov %cr4, %eax
    or $0x20, %eax               /* CR4.PAE */
    mov %eax, %cr4

    /* Load PML4 into CR3 */
    mov $pml4, %eax
    mov %eax, %cr3

    /* Enable long mode via EFER.LME */
    mov $0xC0000080, %ecx        /* IA32_EFER */
    rdmsr
    or $0x00000100, %eax         /* LME bit */
    wrmsr

    /* Enable paging (CR0.PG) */
    mov %cr0, %eax
    or $0x80000000, %eax         /* PG */
    mov %eax, %cr0

    /* Debug: Output before ljmp to long mode */
    mov $0x3F8, %dx
    mov $'P', %al
    out %al, %dx

    /* Far jump to 64-bit code segment (selector 0x08) */
    ljmp $0x08, $long_mode_entry

/* ----------------------- 64-bit mode ------------------------- */
.code64
long_mode_entry:
    /* Debug: Raw byte to COM1 to confirm we reach 64-bit mode */
    mov $0x3F8, %dx
    mov $'L', %al
    out %al, %dx

    /* Set up 64-bit stack */
    mov $stack64_top, %rsp

    /* Debug: Another byte before calling C code */
    mov $0x3F8, %dx
    mov $'M', %al
    out %al, %dx

    /* Call C kernel entry */
    .extern kernel_main
    /* Move saved multiboot values into SysV ABI arg registers */
    movzx mb_magic32, %esi        /* RSI = magic (zero-extend 32-bit) */
    mov mb_info32, %edi           /* RDI = info pointer (zero-extend by mov) */
    call kernel_main

.hang:
    hlt
    jmp .hang

/* ----------------------- GDT ------------------------- */
.align 8
gdt64:
    /* Null */
    .quad 0x0000000000000000
    /* 64-bit code segment: base=0, limit=0xFFFFF (ignored), flags set: G=1, L=1, P=1, DPL=0, Code=1, Readable=1 */
    .quad 0x00AF9A000000FFFF
    /* 64-bit data segment: base=0, limit=0xFFFFF, G=1, P=1, Writable=1 */
    .quad 0x00AF92000000FFFF

gdt_descriptor:
    .word (gdt_end - gdt64 - 1)
    .long gdt64                 /* In 32-bit mode, 32-bit pointer is fine here */
gdt_end:

/* ----------------------- Page tables ------------------------- */
.section .data
.align 4096
.globl pml4
pml4:
    .quad pdpt + 0x03           /* present | rw */
    .fill 511,8,0

.align 4096
.globl pdpt
pdpt:
    .quad pd + 0x03             /* present | rw */
    .fill 511,8,0

.align 4096
.globl pd
pd:
    /* 2 MiB identity-mapped page for 0x00000000 - 0x001FFFFF; flags: present|rw|PS */
    .quad 0x0000000000000083
    .fill 511,8,0

/* ----------------------- Stacks ------------------------- */
.section .bss
.align 16
.lcomm stack32, 4096
.set stack32_top, stack32 + 4096

.align 16
.lcomm stack64, 16384
.set stack64_top, stack64 + 16384

/* Saved multiboot registers from 32-bit stage */
.section .data
.globl mb_info32
.globl mb_magic32
mb_info32:
    .long 0
mb_magic32:
    .long 0
