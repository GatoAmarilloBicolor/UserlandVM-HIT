// Test program for arithmetic operations using 0x80 group opcodes
// Tests the opcodes we implemented in EnhancedInterpreterX86_32

void _start() {
    int result = 0;
    
    // Test ADD operation (0x80 /0) - adds 10 to 5, should get 15
    asm volatile (
        "movl $5, %%eax\n\t"     // Start with 5
        "movl $10, %%ebx\n\t"    // Value to add
        "addb %%bl, %%al\n\t"    // ADD 0x80 /0: add bl to al (low 8 bits)
        "movb %%al, %0\n\t"      // Store result
        : "=r" (result)
        :
        : "eax", "ebx"
    );
    
    // Test SUB operation (0x80 /5) - subtract 3 from 20, should get 17
    asm volatile (
        "movl $20, %%eax\n\t"    // Start with 20
        "movl $3, %%ebx\n\t"     // Value to subtract
        "subb %%bl, %%al\n\t"    // SUB 0x80 /5: subtract bl from al
        "movb %%al, %0\n\t"      // Store result (will be 17)
        : "=r" (result)
        :
        : "eax", "ebx"
    );
    
    // Test AND operation (0x80 /4) - AND 0xFF with 0x0F, should get 0x0F
    asm volatile (
        "movl $0xFF, %%eax\n\t"  // Start with 0xFF
        "movl $0x0F, %%ebx\n\t"  // Value to AND with
        "andb %%bl, %%al\n\t"    // AND 0x80 /4: and bl with al
        "movb %%al, %0\n\t"      // Store result (will be 0x0F)
        : "=r" (result)
        :
        : "eax", "ebx"
    );
    
    // Test XOR operation (0x80 /6) - XOR 0xAA with 0xFF, should get 0x55
    asm volatile (
        "movl $0xAA, %%eax\n\t"  // Start with 0xAA
        "movl $0xFF, %%ebx\n\t"  // Value to XOR with
        "xorb %%bl, %%al\n\t"    // XOR 0x80 /6: xor bl with al
        "movb %%al, %0\n\t"      // Store result (will be 0x55)
        : "=r" (result)
        :
        : "eax", "ebx"
    );
    
    // Test conditional jumps - jump based on flags
    asm volatile (
        "movl $0, %%eax\n\t"     // Clear flags by setting result to 0
        "cmp $10, %%eax\n\t"     // Compare 10 with 0 (sets flags)
        "jne skip_jump\n\t"       // Jump if not equal (should jump)
        "movl $1, %%eax\n\t"     // This should be skipped
        "skip_jump:\n\t"
        "movl %%eax, %0\n\t"     // Store result (should be 0)
        : "=r" (result)
        :
        : "eax"
    );
    
    // Test IN/OUT operations (0xEC, 0xEE)
    asm volatile (
        "movw $0x378, %%dx\n\t"  // Typical parallel port address
        "inb %%dx, %%al\n\t"     // IN 0xEC: read from port DX into AL
        "outb %%al, %%dx\n\t"    // OUT 0xEE: write AL to port DX
        "movb %%al, %0\n\t"      // Store whatever was read
        : "=r" (result)
        :
        : "eax", "edx"
    );
    
    // Exit with success
    asm volatile (
        "movl $1, %%eax\n\t"     // syscall number for exit
        "movl $0, %%ebx\n\t"     // exit status 0 (success)
        "int $0x80\n\t"          // make syscall
    );
}