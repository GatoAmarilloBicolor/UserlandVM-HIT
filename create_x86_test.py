import struct

# Create a valid 32-bit x86 ELF with proper symbols
e_ident = bytearray([
    0x7f, ord('E'), ord('L'), ord('F'),
    1,      # ELFCLASS32
    1,      # ELFDATA2LSB
    1,      # EV_CURRENT
    0,      # ELFOSABI_NONE
    0, 0, 0, 0, 0, 0, 0
])

e_type = 2          # ET_EXEC
e_machine = 3       # EM_386
e_version = 1
e_entry = 0x08048100
e_phoff = 52
e_shoff = 0
e_flags = 0
e_ehsize = 52
e_phentsize = 32
e_phnum = 1
e_shentsize = 0
e_shnum = 0
e_shstrndx = 0

elf_header = e_ident
elf_header += struct.pack('<H', e_type)
elf_header += struct.pack('<H', e_machine)
elf_header += struct.pack('<I', e_version)
elf_header += struct.pack('<I', e_entry)
elf_header += struct.pack('<I', e_phoff)
elf_header += struct.pack('<I', e_shoff)
elf_header += struct.pack('<I', e_flags)
elf_header += struct.pack('<H', e_ehsize)
elf_header += struct.pack('<H', e_phentsize)
elf_header += struct.pack('<H', e_phnum)
elf_header += struct.pack('<H', e_shentsize)
elf_header += struct.pack('<H', e_shnum)
elf_header += struct.pack('<H', e_shstrndx)

# Program header
p_type = 1
p_offset = 84
p_vaddr = e_entry
p_paddr = e_entry
p_filesz = 0x2000
p_memsz = 0x2000
p_flags = 7         # PF_R | PF_W | PF_X
p_align = 0x1000

prog_header = struct.pack('<I', p_type)
prog_header += struct.pack('<I', p_offset)
prog_header += struct.pack('<I', p_vaddr)
prog_header += struct.pack('<I', p_paddr)
prog_header += struct.pack('<I', p_filesz)
prog_header += struct.pack('<I', p_memsz)
prog_header += struct.pack('<I', p_flags)
prog_header += struct.pack('<I', p_align)

# Create code section with symbols and data
# Layout: header(84) + prog(32) = 116 bytes offset
# Padding to reach 0x100 = 256 bytes
padding = b'\x00' * (256 - 84 - 32 - 116)

# Start of actual code/data at 0x08048100 (relative offset 256 from file start)
# First two dwords are gRetProc and gRetProcArg
gRetProc = struct.pack('<I', 0x08048200)      # Dummy value
gRetProcArg = struct.pack('<I', 0x00000000)   # Dummy value

# Rest is NOP instructions (x86: 0x90)
code = gRetProc + gRetProcArg + (b'\x90' * (0x2000 - 8))

# Write file
with open('TestX86', 'wb') as f:
    f.write(elf_header)
    f.write(prog_header)
    f.write(padding)
    f.write(code)

print('Created x86 test ELF: TestX86')
