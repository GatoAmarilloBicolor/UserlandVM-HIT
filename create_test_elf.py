#!/usr/bin/env python3
import struct

# Simple minimal ELF32 header for x86 exit(0)
# This is just to test the loader

with open('test_binary_32bit', 'wb') as f:
    # ELF header
    f.write(b'\x7fELF')  # Magic
    f.write(b'\x01')  # 32-bit
    f.write(b'\x01')  # Little-endian
    f.write(b'\x01')  # ELF version
    f.write(b'\x00' * 9)  # Padding
    
    # rest is stubbed for quick test
    f.write(b'\x00' * 36)

print("✅ Test ELF created: test_binary_32bit")
