# Simple ET_DYN test with relocations

.section .text
.global _start
_start:
    # Simple write syscall test
    movl $4, %eax      # syscall write
    movl $1, %ebx      # stdout
    movl $message, %ecx  # message address
    movl $len, %edx     # message length
    int $0x80
    
    # Exit with success
    movl $1, %eax
    movl $0, %ebx
    int $0x80

.section .data
message:
    .ascii "ET_DYN relocation test successful!\n"
len = . - message