.data
message: .ascii "Hello from Dynamic Haiku Program!\n"
message_len = 33

.global _start
.type _start, @function

.text
_start:
    # Push message address
    pushl $message
    # Push message length
    pushl $message_len
    # Call write syscall (fd=1)
    pushl $1
    call _kern_write
    
    # Exit with success
    movl $41, %eax  # _kern_exit_team
    movl $0, %ebx
    int $0x99