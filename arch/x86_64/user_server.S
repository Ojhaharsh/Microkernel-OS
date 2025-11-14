// User-space server: recv messages and reply with "pong i\n"

.text
.global user_server_start

// rax syscall numbers (per include/syscall.h):
// 1=write, 2=yield, 3=exit, 4=send, 5=recv

user_server_start:
    xor %rbx, %rbx                 // counter i = 0
.Lloop_server:
    // recv(buf, maxlen)
    lea buf(%rip), %rdi            // buf
    mov $128, %rsi                 // maxlen
    mov $5, %rax                   // SYS_recv
    syscall                        // rax = (from<<32) | len

    mov %rax, %rcx                 // save packed
    mov %rax, %rdx
    shr $32, %rcx                  // rcx = from (low32 of high part)
    and $0xFFFFFFFF, %edx          // edx = len (low32)
    // Store reply destination immediately to avoid clobber by later syscalls
    lea pkt(%rip), %r8             // r8 = &pkt
    movq %rcx, 0(%r8)              // pkt.dst = from
    mov %rcx, %r10                 // preserve 'from' across subsequent syscalls

    // Optional: write prefix and payload to serial via SYS_write
    mov $1, %rax                   // SYS_write
    lea pfx_srv(%rip), %rdi        // buf
    mov $pfx_srv_end - pfx_srv, %rsi
    syscall

    mov $1, %rax                   // SYS_write
    lea buf(%rip), %rdi
    mov %rdx, %rsi                 // len
    syscall

    // Build reply "pong i\n" into pkt.data
    inc %rbx                       // i++
    lea pkt(%rip), %r8             // r8 = &pkt
    movq $7, 8(%r8)                // pkt.len = 7
    // pkt.data at 16(%r8)
    movb $'p', 16(%r8)
    movb $'o', 17(%r8)
    movb $'n', 18(%r8)
    movb $'g', 19(%r8)
    movb $' ', 20(%r8)
    // digit = (i % 10) + '0'
    mov %rbx, %rax
    xor %rdx, %rdx
    mov $10, %r9
    divq %r9                        // rax = i/10, rdx = i%10
    add $'0', %dl
    mov %dl, 21(%r8)
    movb $'\n', 22(%r8)

    // send(&pkt)
    mov $4, %rax                   // SYS_send
    lea pkt(%rip), %rdi
    syscall

    // debug: confirm reply sent
    mov $1, %rax                   // SYS_write
    lea pfx_srv_sent(%rip), %rdi
    mov $pfx_srv_sent_end - pfx_srv_sent, %rsi
    syscall

    // if send rc < 0, print failure
    test %rax, %rax
    jns 1f
    mov $1, %rax
    lea pfx_srv_fail(%rip), %rdi
    mov $pfx_srv_fail_end - pfx_srv_fail, %rsi
    syscall
1:

    jmp .Lloop_server

.data
pfx_srv:
    .asciz "[server] got "
pfx_srv_end:

pfx_srv_sent:
    .asciz "[server] replied\n"
pfx_srv_sent_end:

pfx_srv_fail:
    .asciz "[server] send failed\n"
pfx_srv_fail_end:

.bss
.align 16
buf: .zero 128
.align 16
pkt: .zero 16 + 128               // dst(8) + len(8) + data[128]
