// User-space client: send "ping i\n" to server (id 0), recv reply, print

.text
.global user_client_start

// syscall numbers: 1=write, 2=yield, 3=exit, 4=send, 5=recv

user_client_start:
    // debug: client entry
    mov $1, %rax                   // SYS_write
    lea cli_start(%rip), %rdi
    mov $cli_start_end - cli_start, %rsi
    syscall
    xor %rbx, %rbx                 // i = 0
    mov $0, %r12                   // server_id = 0 (first user task)
.Lloop_client:
    inc %rbx                       // i++
    // Build "ping i\n" into pkt.data
    lea pkt(%rip), %r8             // r8 = &pkt
    movq %r12, 0(%r8)              // pkt.dst = server_id
    movq $7, 8(%r8)                // pkt.len = 7
    movb $'p', 16(%r8)
    movb $'i', 17(%r8)
    movb $'n', 18(%r8)
    movb $'g', 19(%r8)
    movb $' ', 20(%r8)
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

    // debug: after send
    mov $1, %rax                   // SYS_write
    lea cli_sent(%rip), %rdi
    mov $cli_sent_end - cli_sent, %rsi
    syscall

    // recv reply into buf
    // debug: announce waiting for reply
    mov $1, %rax                   // SYS_write
    lea pfx_cli_wait(%rip), %rdi
    mov $pfx_cli_wait_end - pfx_cli_wait, %rsi
    syscall

    lea buf(%rip), %rdi
    mov $128, %rsi
    mov $5, %rax                   // SYS_recv
    syscall                        // rax=(from<<32)|len
    mov %rax, %rdx
    and $0xFFFFFFFF, %edx          // len

    // print prefix and payload
    mov $1, %rax                   // SYS_write
    lea pfx_cli(%rip), %rdi
    mov $pfx_cli_end - pfx_cli, %rsi
    syscall

    mov $1, %rax                   // SYS_write
    lea buf(%rip), %rdi
    mov %rdx, %rsi                 // len
    syscall

    // loop 5 times then exit
    cmp $5, %rbx
    jl .Lloop_client

    mov $3, %rax                   // SYS_exit
    syscall

.data
pfx_cli:
    .asciz "[client] got "
pfx_cli_end:

cli_start:
    .asciz "[client] start\n"
cli_start_end:

cli_sent:
    .asciz "[client] sent\n"
cli_sent_end:

pfx_cli_wait:
    .asciz "[client] waiting for reply\n"
pfx_cli_wait_end:

.bss
.align 16
buf: .zero 128
.align 16
pkt: .zero 16 + 128               // dst(8) + len(8) + data[128]
