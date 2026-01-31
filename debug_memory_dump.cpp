// Simple debug program to dump memory content and compare with file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main() {
    // Read libroot.so file
    FILE* f = fopen("sysroot/haiku32/system/lib/libroot.so", "rb");
    if (!f) {
        perror("Cannot open libroot.so");
        return 1;
    }
    
    // Seek to offset 0x32838 (which maps to guest address 0x40143250)
    // libroot loads at 0x4010ba18, and this address is 0x40143250
    // offset = 0x40143250 - 0x4010ba18 = 0x32838
    
    uint32_t guest_addr = 0x40143250;
    uint32_t load_base = 0x4010ba18;
    uint32_t file_offset = guest_addr - load_base;
    
    printf("Guest address:  0x%08x\n", guest_addr);
    printf("Load base:      0x%08x\n", load_base);
    printf("File offset:    0x%08x\n", file_offset);
    printf("\n");
    
    // Read bytes from file
    fseek(f, file_offset, SEEK_SET);
    uint8_t file_bytes[64];
    size_t read = fread(file_bytes, 1, sizeof(file_bytes), f);
    
    printf("Bytes from libroot.so file at offset 0x%08x:\n", file_offset);
    printf("  ");
    for (size_t i = 0; i < read && i < 32; i++) {
        printf("%02x ", file_bytes[i]);
    }
    printf("\n\n");
    
    // Expected: instruction at this address should be sensible
    printf("Expected pattern:\n");
    printf("  Common opcodes: 55 (PUSH), 89 (MOV), 8B (MOV), E8 (CALL), C3 (RET)\n");
    printf("  At offset 0x32838, file contains: %02x %02x %02x %02x %02x ...\n",
        file_bytes[0], file_bytes[1], file_bytes[2], file_bytes[3], file_bytes[4]);
    
    // The problematic case: if the first byte is E8, it's a CALL
    if (file_bytes[0] == 0xE8) {
        int32_t offset = *(int32_t*)&file_bytes[1];
        uint32_t target = guest_addr + 5 + offset;
        printf("\n  This IS a CALL: E8 offset=0x%08x → target=0x%08x\n", offset, target);
        if (target < 0x40000000 || target > 0x41000000) {
            printf("    ⚠️  WARNING: Target 0x%08x is outside normal range!\n", target);
        }
    } else {
        printf("\n  First byte 0x%02x is NOT E8 (CALL opcode)\n", file_bytes[0]);
        printf("    If execution reads E8 here, the memory is corrupted!\n");
    }
    
    fclose(f);
    return 0;
}
