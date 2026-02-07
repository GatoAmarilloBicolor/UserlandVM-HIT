; Simple test program for Phase 2
; Only uses write() and exit() syscalls
; Entry point at 0x08048000

[BITS 32]
[ORG 0x08048000]

  mov eax, 4           ; syscall: write
  mov ebx, 1           ; fd = stdout
  mov ecx, message     ; buf = message
  mov edx, msg_len     ; count = msg_len
  int 0x80             ; syscall

  mov eax, 1           ; syscall: exit
  mov ebx, 0           ; code = 0
  int 0x80             ; syscall

message: db "Hello from Phase 2!", 0x0a
msg_len equ $ - message

times 4096 - $ db 0   ; Pad to 4KB
