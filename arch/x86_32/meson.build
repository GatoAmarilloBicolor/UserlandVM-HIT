# x86-32 architecture implementation

x86_32_sources = [
    'X86_32GuestContext.cpp',
    'InterpreterX86_32.cpp',
    'X86_32Haiku32SyscallDispatcher.cpp',
]

x86_32_headers = [
    'X86_32GuestContext.h',
    'InterpreterX86_32.h',
    'X86_32Haiku32SyscallDispatcher.h',
]

x86_32_lib = static_library(
    'uvm_x86_32',
    x86_32_sources,
    link_with: core_lib,
    install: false
)
