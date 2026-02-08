// Simple test program to validate write syscall
// Compile with: gcc -m32 -nostdlib -static test_write.c -o test_write

void _start() {
    const char* message = "Hello from UserlandVM write syscall test!\n";
    int length = 41;  // Length of the message
    
    // syscall 4 = write, fd 1 = stdout
    asm volatile (
        "movl $4, %%eax\n"      // syscall number for write
        "movl $1, %%ebx\n"      // file descriptor 1 (stdout)
        "movl %0, %%ecx\n"      // message address
        "movl %1, %%edx\n"      // message length
        "int $0x80\n"           // make syscall
        :
        : "r" (message), "r" (length)
        : "eax", "ebx", "ecx", "edx"
    );
    
    // syscall 1 = exit, status 0
    asm volatile (
        "movl $1, %%eax\n"      // syscall number for exit
        "movl $0, %%ebx\n"      // exit status 0
        "int $0x80\n"           // make syscall
    );
}