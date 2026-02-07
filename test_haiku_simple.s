.section .text
.globl _start
.type _start, @function

_start:
    # Haiku x86-32 syscall test
    # syscall 151 = _kern_write
    mov $0x97, %eax    # 151 decimal = 0x97
    mov $1, %ebx       # fd = 1 (stdout)
    mov $message, %ecx # buffer
    mov $message_len, %edx # count
    
    int $0x99          # Haiku syscall interrupt
    
    # syscall 41 = _kern_exit_team  
    mov $0x29, %eax    # 41 decimal = 0x29
    mov $42, %ebx      # exit code
    
    int $0x99          # Haiku syscall interrupt

.section .data
message: .ascii "Hello from Haiku VM!\n"
message_len = . - message