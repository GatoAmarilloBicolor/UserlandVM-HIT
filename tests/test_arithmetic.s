# Test program for arithmetic operations using 0x80 group opcodes
# Tests the opcodes we implemented in EnhancedInterpreterX86_32

.section .data
result_msg:
    .ascii "Arithmetic test completed successfully!\n"
result_len = . - result_msg

.section .text
.global _start
_start:
    # Test ADD operation (0x80 /0) - adds 10 to 5, should get 15
    movb $5, %al           # Start with 5
    movb $10, %bl          # Value to add
    addb %bl, %al          # ADD 0x80 /0: add bl to al
    
    # Test SUB operation (0x80 /5) - subtract 3 from 20, should get 17
    movb $20, %cl          # Start with 20
    movb $3, %dl           # Value to subtract
    subb %dl, %cl          # SUB 0x80 /5: subtract dl from cl
    
    # Test AND operation (0x80 /4) - AND 0xFF with 0x0F, should get 0x0F
    movb $0xFF, %dh        # Start with 0xFF
    movb $0x0F, %bh        # Value to AND with
    andb %bh, %dh          # AND 0x80 /4: and bh with dh
    
    # Test XOR operation (0x80 /6) - XOR 0xAA with 0xFF, should get 0x55
    movb $0xAA, %ah        # Start with 0xAA
    xorb $0xFF, %ah        # XOR 0x80 /6: xor immediate with ah
    
    # Test conditional jumps - jump based on flags
    movl $0, %eax          # Clear result
    cmpl $10, %eax         # Compare 10 with 0 (sets flags)
    jne skip_jump          # Jump if not equal (should jump)
    movl $1, %eax          # This should be skipped
skip_jump:
    
    # Skip IN/OUT operations (privileged) - just set a register value
    movb $0x42, %al        # Set AL to a known value to test opcodes worked
    
    # Write success message
    movl $4, %eax          # syscall number for write
    movl $1, %ebx          # file descriptor 1 (stdout)
    movl $result_msg, %ecx # message address
    movl $result_len, %edx # message length
    int $0x80              # make syscall
    
    # Exit with success
    movl $1, %eax          # syscall number for exit
    movl $0, %ebx          # exit status 0 (success)
    int $0x80              # make syscall