.data
message: .ascii "Hello from Dynamic Haiku Program!\n"
message_len = . - message

.global _start
.type _start, @function

.text
_start:
    # Call external function (dynamic linking test)
    call printf
    
    # Exit with success
    movl $0, %eax
    pushl %eax
    call exit