// Simple test program to validate write syscall using assembly
// This will be created with assembly language directly

.section .data
message:
    .ascii "Hello from UserlandVM write syscall test!\n"
len = . - message

.section .text
.global _start
_start:
    # syscall 4 = write, fd 1 = stdout
    movl $4, %eax          # syscall number for write
    movl $1, %ebx          # file descriptor 1 (stdout)
    movl $message, %ecx    # message address
    movl $len, %edx        # message length
    int $0x80              # make syscall
    
    # syscall 1 = exit, status 0
    movl $1, %eax          # syscall number for exit
    movl $0, %ebx          # exit status 0
    int $0x80              # make syscall