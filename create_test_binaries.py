#!/usr/bin/env python3
"""
Create valid 32-bit x86 ELF binaries for testing UserlandVM
"""
import struct
import os

def create_hello_world_elf32():
    """Create a simple Hello World ELF32 binary"""
    
    # x86-32 Assembly code for hello world
    # This will be placed at 0x08048000
    code = bytes([
        # write(1, "Hello, World!\n", 14)
        0xb8, 0x04, 0x00, 0x00, 0x00,  # mov eax, 4 (write)
        0xbb, 0x01, 0x00, 0x00, 0x00,  # mov ebx, 1 (stdout)
        0xb9, 0x22, 0x80, 0x04, 0x08,  # mov ecx, 0x08048022 (msg address)
        0xba, 0x0e, 0x00, 0x00, 0x00,  # mov edx, 14 (msg length)
        0xcd, 0x80,                      # int 0x80 (syscall)
        
        # exit(0)
        0xb8, 0x01, 0x00, 0x00, 0x00,  # mov eax, 1 (exit)
        0xbb, 0x00, 0x00, 0x00, 0x00,  # mov ebx, 0 (exit code)
        0xcd, 0x80,                      # int 0x80 (syscall)
    ])
    
    message = b"Hello, World!\n"
    
    # ELF32 Header (52 bytes)
    elf_header = bytes([
        0x7f, 0x45, 0x4c, 0x46,        # e_ident[0:4] - ELF magic
        0x01,                            # e_ident[4] - 32-bit
        0x01,                            # e_ident[5] - little endian
        0x01,                            # e_ident[6] - ELF version
        0x00,                            # e_ident[7] - System V ABI
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # padding
    ])
    
    elf_header += struct.pack('<H', 2)   # e_type - ET_EXEC
    elf_header += struct.pack('<H', 3)   # e_machine - EM_386 (x86)
    elf_header += struct.pack('<I', 1)   # e_version
    elf_header += struct.pack('<I', 0x08048000)  # e_entry (entry point)
    elf_header += struct.pack('<I', 52)  # e_phoff (program header offset)
    elf_header += struct.pack('<I', 0)   # e_shoff (section header offset)
    elf_header += struct.pack('<I', 0)   # e_flags
    elf_header += struct.pack('<H', 52)  # e_ehsize
    elf_header += struct.pack('<H', 32)  # e_phentsize
    elf_header += struct.pack('<H', 1)   # e_phnum (number of program headers)
    elf_header += struct.pack('<H', 0)   # e_shentsize
    elf_header += struct.pack('<H', 0)   # e_shnum
    elf_header += struct.pack('<H', 0)   # e_shstrndx
    
    # Program Header (32 bytes) - PT_LOAD
    prog_header = struct.pack('<I', 1)   # p_type - PT_LOAD
    prog_header += struct.pack('<I', 52) # p_offset
    prog_header += struct.pack('<I', 0x08048000)  # p_vaddr
    prog_header += struct.pack('<I', 0x08048000)  # p_paddr
    prog_header += struct.pack('<I', len(code) + len(message))  # p_filesz
    prog_header += struct.pack('<I', len(code) + len(message))  # p_memsz
    prog_header += struct.pack('<I', 7)  # p_flags (R+W+X)
    prog_header += struct.pack('<I', 0x1000)  # p_align
    
    binary = elf_header + prog_header + code + message
    return binary

def create_file_read_test_elf32():
    """Create ELF32 binary that reads /etc/profile"""
    
    # x86-32 Assembly code
    code = bytes([
        # open("/etc/profile", O_RDONLY)
        0xb8, 0x05, 0x00, 0x00, 0x00,  # mov eax, 5 (open)
        0xb9, 0x32, 0x80, 0x04, 0x08,  # mov ecx, 0x08048032 (path addr)
        0xba, 0x00, 0x00, 0x00, 0x00,  # mov edx, 0 (O_RDONLY)
        0xcd, 0x80,                      # int 0x80
        
        # read(fd, buf, 256)
        0x89, 0xc3,                      # mov ebx, eax (fd in ebx)
        0xb8, 0x03, 0x00, 0x00, 0x00,  # mov eax, 3 (read)
        0xb9, 0x32, 0x81, 0x04, 0x08,  # mov ecx, 0x08048132 (buf addr)
        0xba, 0x00, 0x01, 0x00, 0x00,  # mov edx, 256 (count)
        0xcd, 0x80,                      # int 0x80
        
        # write(1, buf, returned_count)
        0x89, 0xc2,                      # mov edx, eax (ret count)
        0xb8, 0x04, 0x00, 0x00, 0x00,  # mov eax, 4 (write)
        0xbb, 0x01, 0x00, 0x00, 0x00,  # mov ebx, 1 (stdout)
        0xb9, 0x32, 0x81, 0x04, 0x08,  # mov ecx, 0x08048132 (buf addr)
        0xcd, 0x80,                      # int 0x80
        
        # close(fd)
        0xb8, 0x06, 0x00, 0x00, 0x00,  # mov eax, 6 (close)
        # ebx still has fd
        0xcd, 0x80,                      # int 0x80
        
        # exit(0)
        0xb8, 0x01, 0x00, 0x00, 0x00,  # mov eax, 1 (exit)
        0xbb, 0x00, 0x00, 0x00, 0x00,  # mov ebx, 0 (exit code)
        0xcd, 0x80,                      # int 0x80
    ])
    
    path = b"/etc/profile\x00"
    buf = b"\x00" * 256
    
    # ELF32 Header (52 bytes)
    elf_header = bytes([
        0x7f, 0x45, 0x4c, 0x46,        # e_ident[0:4] - ELF magic
        0x01,                            # e_ident[4] - 32-bit
        0x01,                            # e_ident[5] - little endian
        0x01,                            # e_ident[6] - ELF version
        0x00,                            # e_ident[7] - System V ABI
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # padding
    ])
    
    elf_header += struct.pack('<H', 2)   # e_type - ET_EXEC
    elf_header += struct.pack('<H', 3)   # e_machine - EM_386
    elf_header += struct.pack('<I', 1)   # e_version
    elf_header += struct.pack('<I', 0x08048000)  # e_entry
    elf_header += struct.pack('<I', 52)  # e_phoff
    elf_header += struct.pack('<I', 0)   # e_shoff
    elf_header += struct.pack('<I', 0)   # e_flags
    elf_header += struct.pack('<H', 52)  # e_ehsize
    elf_header += struct.pack('<H', 32)  # e_phentsize
    elf_header += struct.pack('<H', 1)   # e_phnum
    elf_header += struct.pack('<H', 0)   # e_shentsize
    elf_header += struct.pack('<H', 0)   # e_shnum
    elf_header += struct.pack('<H', 0)   # e_shstrndx
    
    # Program Header (32 bytes) - PT_LOAD
    prog_header = struct.pack('<I', 1)   # p_type
    prog_header += struct.pack('<I', 52) # p_offset
    prog_header += struct.pack('<I', 0x08048000)  # p_vaddr
    prog_header += struct.pack('<I', 0x08048000)  # p_paddr
    prog_header += struct.pack('<I', len(code) + len(path) + len(buf))  # p_filesz
    prog_header += struct.pack('<I', len(code) + len(path) + len(buf))  # p_memsz
    prog_header += struct.pack('<I', 7)  # p_flags
    prog_header += struct.pack('<I', 0x1000)  # p_align
    
    binary = elf_header + prog_header + code + path + buf
    return binary

# Create binaries
print("[*] Creating test ELF32 binaries...")

hello = create_hello_world_elf32()
with open("test_programs/hello_world_elf32", "wb") as f:
    f.write(hello)
print(f"[+] Created hello_world_elf32: {len(hello)} bytes")

read_test = create_file_read_test_elf32()
with open("test_programs/read_file_elf32", "wb") as f:
    f.write(read_test)
print(f"[+] Created read_file_elf32: {len(read_test)} bytes")

print("[+] Done!")
