#!/usr/bin/env python3
"""
Create valid 32-bit x86 ELF binaries based on known working formats.
These are created by directly authoring ELF headers and assembly code.
"""
import struct

def create_minimal_elf32(code_bytes, message=b""):
    """
    Create a minimal but valid ELF32 binary.
    
    Layout:
    - ELF Header: 52 bytes at offset 0x00
    - Program Header: 32 bytes at offset 0x34
    - Code+Data: starting at offset 0x54 (virtual address 0x08048054)
    """
    
    elf_header_size = 52  # ELF header size
    prog_header_offset = elf_header_size  # Program header comes right after ELF header
    prog_header_size = 32  # One program header
    code_file_offset = prog_header_offset + prog_header_size  # Code comes after headers
    code_vaddr = 0x08048000 + code_file_offset  # Virtual address = base + offset
    vaddr_base = 0x08048000  # Virtual address base
    
    # ELF32 Header (52 bytes total)
    header = bytearray(52)
    header[0:4] = b'\x7fELF'           # e_ident[0:4] - ELF magic
    header[4] = 0x01                   # e_ident[4] - 32-bit
    header[5] = 0x01                   # e_ident[5] - little endian
    header[6] = 0x01                   # e_ident[6] - ELF version
    header[7] = 0x00                   # e_ident[7] - System V ABI
    # header[8:16] already zero (padding)
    
    offset = 16
    header[offset:offset+2] = struct.pack('<H', 2)      # e_type = ET_EXEC
    header[offset+2:offset+4] = struct.pack('<H', 3)    # e_machine = EM_386
    header[offset+4:offset+8] = struct.pack('<I', 1)    # e_version
    header[offset+8:offset+12] = struct.pack('<I', code_vaddr)  # e_entry
    header[offset+12:offset+16] = struct.pack('<I', prog_header_offset)  # e_phoff
    header[offset+16:offset+20] = struct.pack('<I', 0)  # e_shoff (no sections)
    header[offset+20:offset+24] = struct.pack('<I', 0)  # e_flags
    header[offset+24:offset+26] = struct.pack('<H', 52) # e_ehsize
    header[offset+26:offset+28] = struct.pack('<H', 32) # e_phentsize
    header[offset+28:offset+30] = struct.pack('<H', 1)  # e_phnum
    header[offset+30:offset+32] = struct.pack('<H', 0)  # e_shentsize
    header[offset+32:offset+34] = struct.pack('<H', 0)  # e_shnum
    header[offset+34:offset+36] = struct.pack('<H', 0)  # e_shstrndx
    
    # Program Header (32 bytes) - PT_LOAD
    pheader = bytearray(32)
    offset = 0
    pheader[offset:offset+4] = struct.pack('<I', 1)     # p_type = PT_LOAD
    pheader[offset+4:offset+8] = struct.pack('<I', code_file_offset)  # p_offset
    pheader[offset+8:offset+12] = struct.pack('<I', code_vaddr)  # p_vaddr
    pheader[offset+12:offset+16] = struct.pack('<I', code_vaddr)  # p_paddr
    total_size = len(code_bytes) + len(message)
    pheader[offset+16:offset+20] = struct.pack('<I', total_size)  # p_filesz
    pheader[offset+20:offset+24] = struct.pack('<I', total_size)  # p_memsz
    pheader[offset+24:offset+28] = struct.pack('<I', 7)  # p_flags (RWX)
    pheader[offset+28:offset+32] = struct.pack('<I', 0x1000)  # p_align
    
    # Combine all parts
    binary = bytes(header) + bytes(pheader) + code_bytes + message
    return binary

def create_hello_elf32():
    """Create a Hello World program"""
    hello_msg = b"Hello, World!\n"
    # Message starts at: 0x08048054 (code start) + len(code) = 0x08048054 + 34 = 0x08048076
    hello_code = bytes([
        # mov eax, 4          ; syscall write
        0xb8, 0x04, 0x00, 0x00, 0x00,
        # mov ebx, 1          ; fd = stdout
        0xbb, 0x01, 0x00, 0x00, 0x00,
        # mov ecx, <msg_addr> ; msg address
        0xb9, 0x76, 0x80, 0x04, 0x08,  # 0x08048076
        # mov edx, 14         ; msg length
        0xba, 0x0e, 0x00, 0x00, 0x00,
        # int 0x80            ; syscall
        0xcd, 0x80,
        # mov eax, 1          ; syscall exit
        0xb8, 0x01, 0x00, 0x00, 0x00,
        # mov ebx, 0          ; exit code
        0xbb, 0x00, 0x00, 0x00, 0x00,
        # int 0x80            ; syscall
        0xcd, 0x80,
    ])
    return create_minimal_elf32(hello_code, hello_msg)

def create_stderr_elf32():
    """Create a stderr test program"""
    stderr_msg = b"Stderr Test!\n"
    # Message starts at: 0x08048054 + 34 = 0x08048076
    stderr_code = bytes([
        # mov eax, 4          ; syscall write
        0xb8, 0x04, 0x00, 0x00, 0x00,
        # mov ebx, 2          ; fd = stderr
        0xbb, 0x02, 0x00, 0x00, 0x00,
        # mov ecx, <msg_addr> ; msg address
        0xb9, 0x76, 0x80, 0x04, 0x08,  # 0x08048076
        # mov edx, 13         ; msg length
        0xba, 0x0d, 0x00, 0x00, 0x00,
        # int 0x80            ; syscall
        0xcd, 0x80,
        # mov eax, 1          ; syscall exit
        0xb8, 0x01, 0x00, 0x00, 0x00,
        # mov ebx, 0          ; exit code
        0xbb, 0x00, 0x00, 0x00, 0x00,
        # int 0x80            ; syscall
        0xcd, 0x80,
    ])
    return create_minimal_elf32(stderr_code, stderr_msg)

def create_multiple_writes_elf32():
    """Create a program that writes multiple times"""
    msg1 = b"Line 1\n"
    msg2 = b"Line 2\n"
    # First message at code + 34 = 0x08048076
    # Second message at 0x08048076 + len(msg1) = 0x0804807d
    multiple_code = bytes([
        # Write first message
        # mov eax, 4
        0xb8, 0x04, 0x00, 0x00, 0x00,
        # mov ebx, 1
        0xbb, 0x01, 0x00, 0x00, 0x00,
        # mov ecx, 0x08048076
        0xb9, 0x76, 0x80, 0x04, 0x08,
        # mov edx, 7
        0xba, 0x07, 0x00, 0x00, 0x00,
        # int 0x80
        0xcd, 0x80,
        
        # Write second message
        # mov eax, 4
        0xb8, 0x04, 0x00, 0x00, 0x00,
        # mov ebx, 1
        0xbb, 0x01, 0x00, 0x00, 0x00,
        # mov ecx, 0x0804807d (0x08048076 + 7)
        0xb9, 0x7d, 0x80, 0x04, 0x08,
        # mov edx, 7
        0xba, 0x07, 0x00, 0x00, 0x00,
        # int 0x80
        0xcd, 0x80,
        
        # Exit
        # mov eax, 1
        0xb8, 0x01, 0x00, 0x00, 0x00,
        # mov ebx, 0
        0xbb, 0x00, 0x00, 0x00, 0x00,
        # int 0x80
        0xcd, 0x80,
    ])
    return create_minimal_elf32(multiple_code, msg1 + msg2)

print("[*] Creating test ELF32 binaries...")
with open("test_programs/hello_beos_32bit", "wb") as f:
    f.write(create_hello_elf32())
print("[+] Created hello_beos_32bit")

with open("test_programs/stderr_beos_32bit", "wb") as f:
    f.write(create_stderr_elf32())
print("[+] Created stderr_beos_32bit")

with open("test_programs/multiple_writes_32bit", "wb") as f:
    f.write(create_multiple_writes_elf32())
print("[+] Created multiple_writes_32bit")

print("[+] All BeOS 32-bit test binaries created successfully!")
print("[+] Test with: ./build.x86_64/UserlandVM test_programs/<name>")
