#!/usr/bin/env python3
import struct
import os

# Create a minimal valid RISCV64 ELF file

# ELF Header for RISCV64
e_ident = bytearray([
    0x7f, ord('E'), ord('L'), ord('F'),  # ELF magic
    2,      # ELFCLASS64
    1,      # ELFDATA2LSB (little endian)
    1,      # EV_CURRENT
    0,      # ELFOSABI_NONE
    0,      # ABI version
    0, 0, 0, 0, 0, 0, 0  # padding
])

# ELF64 Header structure
e_type = 2          # ET_EXEC
e_machine = 243     # EM_RISCV
e_version = 1       # EV_CURRENT
e_entry = 0x400000
e_phoff = 64        # Program header offset (right after ELF header)
e_shoff = 0         # Section header offset (none needed)
e_flags = 0         # No flags
e_ehsize = 64       # ELF header size
e_phentsize = 56    # Program header entry size
e_phnum = 1         # Number of program headers
e_shentsize = 0
e_shnum = 0
e_shstrndx = 0

# Build ELF header
elf_header = e_ident
elf_header += struct.pack('<H', e_type)
elf_header += struct.pack('<H', e_machine)
elf_header += struct.pack('<I', e_version)
elf_header += struct.pack('<Q', e_entry)
elf_header += struct.pack('<Q', e_phoff)
elf_header += struct.pack('<Q', e_shoff)
elf_header += struct.pack('<I', e_flags)
elf_header += struct.pack('<H', e_ehsize)
elf_header += struct.pack('<H', e_phentsize)
elf_header += struct.pack('<H', e_phnum)
elf_header += struct.pack('<H', e_shentsize)
elf_header += struct.pack('<H', e_shnum)
elf_header += struct.pack('<H', e_shstrndx)

# Program header (single LOAD segment)
p_type = 1          # PT_LOAD
p_flags = 5         # PF_R | PF_X
p_offset = 120      # After headers
p_vaddr = e_entry
p_paddr = e_entry
p_filesz = 4        # 4 bytes of data
p_memsz = 4
p_align = 0x1000

prog_header = struct.pack('<I', p_type)
prog_header += struct.pack('<I', p_flags)
prog_header += struct.pack('<Q', p_offset)
prog_header += struct.pack('<Q', p_vaddr)
prog_header += struct.pack('<Q', p_paddr)
prog_header += struct.pack('<Q', p_filesz)
prog_header += struct.pack('<Q', p_memsz)
prog_header += struct.pack('<Q', p_align)

# Minimal executable code (RISCV64 NOP)
code = b'\x13\x00\x00\x00'  # addi x0, x0, 0

# Write the file
with open('/boot/home/src/UserlandVM-HIT/runtime_loader.riscv64', 'wb') as f:
    f.write(elf_header)
    f.write(prog_header)
    f.write(code)

print("Created minimal RISCV64 ELF: /boot/home/src/UserlandVM-HIT/runtime_loader.riscv64")
print(f"File size: {os.path.getsize('/boot/home/src/UserlandVM-HIT/runtime_loader.riscv64')} bytes")
