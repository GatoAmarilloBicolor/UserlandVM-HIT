#!/usr/bin/env python3

import sys

OPCODES = {
    # MOV instructions
    0x88: ("MOV r8, r/m8", "mov_r8_rm8"),
    0x89: ("MOV r32, r/m32", "mov_r32_rm32"),
    0x8A: ("MOV r/m8, r8", "mov_rm8_r8"),
    0x8B: ("MOV r/m32, r32", "mov_rm32_r32"),
    0x8C: ("MOV r/m16, sreg", "unsupported"),
    0x8E: ("MOV sreg, r/m16", "unsupported"),
    0xA0: ("MOV AL, moffs8", "mov_al_moffs8"),
    0xA1: ("MOV EAX, moffs32", "mov_eax_moffs32"),
    0xA2: ("MOV moffs8, AL", "mov_moffs8_al"),
    0xA3: ("MOV moffs32, EAX", "mov_moffs32_eax"),
    0xB0: ("MOV AL, imm8", "mov_al_imm8"),
    0xB1: ("MOV CL, imm8", "mov_cl_imm8"),
    0xB2: ("MOV DL, imm8", "mov_dl_imm8"),
    0xB3: ("MOV BL, imm8", "mov_bl_imm8"),
    0xB4: ("MOV AH, imm8", "mov_ah_imm8"),
    0xB5: ("MOV CH, imm8", "mov_ch_imm8"),
    0xB6: ("MOV DH, imm8", "mov_dh_imm8"),
    0xB7: ("MOV BH, imm8", "mov_bh_imm8"),
    0xB8: ("MOV EAX, imm32", "mov_eax_imm32"),
    0xB9: ("MOV ECX, imm32", "mov_ecx_imm32"),
    0xBA: ("MOV EDX, imm32", "mov_edx_imm32"),
    0xBB: ("MOV EBX, imm32", "mov_ebx_imm32"),
    0xBC: ("MOV ESP, imm32", "mov_esp_imm32"),
    0xBD: ("MOV EBP, imm32", "mov_ebp_imm32"),
    0xBE: ("MOV ESI, imm32", "mov_esi_imm32"),
    0xBF: ("MOV EDI, imm32", "mov_edi_imm32"),
    0xC6: ("MOV r/m8, imm8", "mov_rm8_imm8"),
    0xC7: ("MOV r/m32, imm32", "mov_rm32_imm32"),
    
    # Arithmetic
    0x00: ("ADD r/m8, r8", "add_rm8_r8"),
    0x01: ("ADD r/m32, r32", "add_rm32_r32"),
    0x02: ("ADD r8, r/m8", "add_r8_rm8"),
    0x03: ("ADD r32, r/m32", "add_r32_rm32"),
    0x04: ("ADD AL, imm8", "add_al_imm8"),
    0x05: ("ADD EAX, imm32", "add_eax_imm32"),
    
    0x28: ("SUB r/m8, r8", "sub_rm8_r8"),
    0x29: ("SUB r/m32, r32", "sub_rm32_r32"),
    0x2A: ("SUB r8, r/m8", "sub_r8_rm8"),
    0x2B: ("SUB r32, r/m32", "sub_r32_rm32"),
    0x2C: ("SUB AL, imm8", "sub_al_imm8"),
    0x2D: ("SUB EAX, imm32", "sub_eax_imm32"),
    
    0x30: ("XOR r/m8, r8", "xor_rm8_r8"),
    0x31: ("XOR r/m32, r32", "xor_rm32_r32"),
    0x32: ("XOR r8, r/m8", "xor_r8_rm8"),
    0x33: ("XOR r32, r/m32", "xor_r32_rm32"),
    0x34: ("XOR AL, imm8", "xor_al_imm8"),
    0x35: ("XOR EAX, imm32", "xor_eax_imm32"),
    
    0x08: ("OR r/m8, r8", "or_rm8_r8"),
    0x09: ("OR r/m32, r32", "or_rm32_r32"),
    0x0A: ("OR r8, r/m8", "or_r8_rm8"),
    0x0B: ("OR r32, r/m32", "or_r32_rm32"),
    0x0C: ("OR AL, imm8", "or_al_imm8"),
    0x0D: ("OR EAX, imm32", "or_eax_imm32"),
    
    0x20: ("AND r/m8, r8", "and_rm8_r8"),
    0x21: ("AND r/m32, r32", "and_rm32_r32"),
    0x22: ("AND r8, r/m8", "and_r8_rm8"),
    0x23: ("AND r32, r/m32", "and_r32_rm32"),
    0x24: ("AND AL, imm8", "and_al_imm8"),
    0x25: ("AND EAX, imm32", "and_eax_imm32"),
    
    0x38: ("CMP r/m8, r8", "cmp_rm8_r8"),
    0x39: ("CMP r/m32, r32", "cmp_rm32_r32"),
    0x3A: ("CMP r8, r/m8", "cmp_r8_rm8"),
    0x3B: ("CMP r32, r/m32", "cmp_r32_rm32"),
    0x3C: ("CMP AL, imm8", "cmp_al_imm8"),
    0x3D: ("CMP EAX, imm32", "cmp_eax_imm32"),
    
    0x84: ("TEST r/m8, r8", "test_rm8_r8"),
    0x85: ("TEST r/m32, r32", "test_rm32_r32"),
    0xA8: ("TEST AL, imm8", "test_al_imm8"),
    0xA9: ("TEST EAX, imm32", "test_eax_imm32"),
    
    # Stack
    0x50: ("PUSH EAX", "push_eax"),
    0x51: ("PUSH ECX", "push_ecx"),
    0x52: ("PUSH EDX", "push_edx"),
    0x53: ("PUSH EBX", "push_ebx"),
    0x54: ("PUSH ESP", "push_esp"),
    0x55: ("PUSH EBP", "push_ebp"),
    0x56: ("PUSH ESI", "push_esi"),
    0x57: ("PUSH EDI", "push_edi"),
    
    0x58: ("POP EAX", "pop_eax"),
    0x59: ("POP ECX", "pop_ecx"),
    0x5A: ("POP EDX", "pop_edx"),
    0x5B: ("POP EBX", "pop_ebx"),
    0x5C: ("POP ESP", "pop_esp"),
    0x5D: ("POP EBP", "pop_ebp"),
    0x5E: ("POP ESI", "pop_esi"),
    0x5F: ("POP EDI", "pop_edi"),
    
    0x68: ("PUSH imm32", "push_imm32"),
    0x6A: ("PUSH imm8", "push_imm8"),
    
    # Control flow
    0xC3: ("RET", "ret"),
    0xC9: ("LEAVE", "leave"),
    0xE8: ("CALL rel32", "call_rel32"),
    0xE9: ("JMP rel32", "jmp_rel32"),
    0xEB: ("JMP rel8", "jmp_rel8"),
    0xFF: ("JMP/CALL r/m32", "grp5_jmp_call"),
    
    # Conditional jumps
    0x70: ("JO rel8", "jo_rel8"),
    0x71: ("JNO rel8", "jno_rel8"),
    0x72: ("JB/JC rel8", "jb_rel8"),
    0x73: ("JAE/JNC rel8", "jae_rel8"),
    0x74: ("JZ/JE rel8", "jz_rel8"),
    0x75: ("JNZ/JNE rel8", "jnz_rel8"),
    0x76: ("JBE/JNA rel8", "jbe_rel8"),
    0x77: ("JA/JNBE rel8", "ja_rel8"),
    0x78: ("JS rel8", "js_rel8"),
    0x79: ("JNS rel8", "jns_rel8"),
    0x7A: ("JP/JPE rel8", "jp_rel8"),
    0x7B: ("JNP/JPO rel8", "jnp_rel8"),
    0x7C: ("JL/JNGE rel8", "jl_rel8"),
    0x7D: ("JGE/JNL rel8", "jge_rel8"),
    0x7E: ("JLE/JNG rel8", "jle_rel8"),
    0x7F: ("JG/JNLE rel8", "jg_rel8"),
    
    # Other
    0x8D: ("LEA r32, m", "lea_r32_m"),
    0x90: ("NOP", "nop"),
    0xCD: ("INT imm8", "int_imm8"),
    0x40: ("INC EAX", "inc_eax"),
    0x41: ("INC ECX", "inc_ecx"),
    0x42: ("INC EDX", "inc_edx"),
    0x43: ("INC EBX", "inc_ebx"),
    0x44: ("INC ESP", "inc_esp"),
    0x45: ("INC EBP", "inc_ebp"),
    0x46: ("INC ESI", "inc_esi"),
    0x47: ("INC EDI", "inc_edi"),
    0x48: ("DEC EAX", "dec_eax"),
    0x49: ("DEC ECX", "dec_ecx"),
    0x4A: ("DEC EDX", "dec_edx"),
    0x4B: ("DEC EBX", "dec_ebx"),
    0x4C: ("DEC ESP", "dec_esp"),
    0x4D: ("DEC EBP", "dec_ebp"),
    0x4E: ("DEC ESI", "dec_esi"),
    0x4F: ("DEC EDI", "dec_edi"),
}

header = '''#pragma once

#include <SupportDefs.h>
#include "X86_32GuestContext.h"
#include "AddressSpace.h"

class SyscallDispatcher;

typedef status_t (*X86Handler)(X86_32GuestContext&, AddressSpace&, SyscallDispatcher&, const uint8_t*, uint32_t&);

struct X86Dispatcher {
    static const X86Handler handlers[256];
    static status_t Execute(uint8_t opcode, X86_32GuestContext& ctx, AddressSpace& space, 
                           SyscallDispatcher& dispatcher, const uint8_t* instr, uint32_t& len);
};

// All handlers
namespace X86Handlers {
'''

footer = '''
}
'''

print(header)
for opcode, (desc, name) in OPCODES.items():
    print(f"    extern status_t {name}(X86_32GuestContext&, AddressSpace&, SyscallDispatcher&, const uint8_t*, uint32_t&);")
    
print(footer)
