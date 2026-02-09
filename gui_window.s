# Simple x86-32 assembly program to create a GUI window via Haiku syscall INT 0x63

.global _start

.data
    title:  .asciz "UserlandVM GUI Window"

.text
_start:
    # create_window syscall (10001)
    # ebx = &title
    # ecx = width (400)
    # edx = height (300)
    
    leal title, %ebx        # ebx = &title
    movl $400, %ecx         # width
    movl $300, %edx         # height
    movl $10001, %eax       # SYSCALL_CREATE_WINDOW
    int $0x63               # Haiku syscall
    
    # Draw a rectangle
    movl $10006, %eax       # SYSCALL_DRAW_RECT
    movl $3250, %ebx        # (50 << 16) | 50 = position
    movl $19660866, %ecx    # (300 << 16) | 200 = size
    int $0x63
    
    # Flush display
    movl $10010, %eax       # SYSCALL_FLUSH
    xorl %ebx, %ebx
    xorl %ecx, %ecx
    xorl %edx, %edx
    int $0x63
    
    # Exit
    movl $1, %eax           # exit syscall
    xorl %ebx, %ebx         # exit code 0
    int $0x80
