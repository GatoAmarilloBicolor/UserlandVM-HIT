# Simple x86-32 Haiku test application
# Demonstrates window creation through Be API bridge

.intel_syntax noprefix
.code32

# Standard x86-32 entry point
.global _start

.text

_start:
    # Initialize stack
    mov esp, 0xbfff8000
    
    # Print welcome message using write syscall (0x4)
    mov eax, 0x4          # write syscall
    mov ebx, 1            # stdout
    lea ecx, [rip + msg1] # address of message
    mov edx, msg1_len     # length
    int 0x80
    
    # Call Create Window (mock syscall)
    # Using a custom GUI syscall (e.g., 0x2710 + 1)
    mov eax, 0x2711      # Create window syscall
    mov ebx, 100         # x position
    mov ecx, 100         # y position
    mov edx, 640         # width
    mov esi, 480         # height
    
    # In real scenario, this would be intercepted by VM
    # For now, we'll just return 0 to continue
    xor eax, eax
    
    # Print execution message
    mov eax, 0x4          # write syscall
    mov ebx, 1            # stdout
    lea ecx, [rip + msg2] # address of message
    mov edx, msg2_len     # length
    int 0x80
    
    # Call exit syscall
    mov eax, 0x1          # exit syscall
    xor ebx, ebx          # exit code 0
    int 0x80

# Data section
.data

msg1:
    .ascii "=== UserlandVM Window Test Application ===\n"
msg1_len = . - msg1

msg2:
    .ascii "[APP] Window syscalls ready for Be API bridge\n"
msg2_len = . - msg2

.bss
stack_space: .space 4096
