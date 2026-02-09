.intel_syntax noprefix
.code32
.global _start

.text

_start:
    # eax = 4 (write syscall)
    # ebx = 1 (stdout fd)
    # ecx = message pointer
    # edx = message length
    
    mov eax, 4
    mov ebx, 1
    mov ecx, offset msg
    mov edx, 40
    int 0x80
    
    # Exit with code 0
    mov eax, 1
    xor ebx, ebx
    int 0x80

.data
msg:
    .ascii "Window test from UserlandVM\n"
