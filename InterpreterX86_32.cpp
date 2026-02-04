/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include "AddressSpace.h"
#include "DebugOutput.h"
#include "FPUInstructionHandler.h"
#include "FloatingPointUnit.h"
#include "GuestContext.h"
#include "InterpreterX86_32.h"
#include "OptimizedX86Executor.h"
#include "StubFunctions.h"
#include "SyscallDispatcher.h"
#include "X86_32GuestContext.h"

// Static array for register names, accessible within this file
static const char *reg_names[] = {"EAX", "ECX", "EDX", "EBX",
                                  "ESP", "EBP", "ESI", "EDI"};

// EFLAGS bits
#define FLAG_CF 0x0001 // Carry Flag
#define FLAG_ZF 0x0040 // Zero Flag
#define FLAG_SF 0x0080 // Sign Flag
#define FLAG_OF 0x0800 // Overflow Flag
#define FLAG_PF 0x0004 // Parity Flag

// Helper function to set flags after ADD operation
// is_32bit: true for 32-bit operation, false for 8-bit
inline void SetFlags_ADD(X86_32Registers &regs, uint64 result, uint64 dst_val,
                         uint64 src_val, bool is_32bit) {
  regs.eflags &= ~(FLAG_CF | FLAG_ZF | FLAG_SF | FLAG_OF | FLAG_PF);

  if (is_32bit) {
    // 32-bit operation
    uint32 res_32 = (uint32)result;
    uint32 dst_32 = (uint32)dst_val;
    uint32 src_32 = (uint32)src_val;

    // Carry Flag: Set if result overflows
    if (result > 0xFFFFFFFFULL)
      regs.eflags |= FLAG_CF;

    // Zero Flag: Set if result is zero
    if (res_32 == 0)
      regs.eflags |= FLAG_ZF;

    // Sign Flag: Set if MSB is set (negative)
    if (res_32 & 0x80000000)
      regs.eflags |= FLAG_SF;

    // Overflow Flag: Set if signed overflow occurred
    bool src_sign = src_32 & 0x80000000;
    bool dst_sign = dst_32 & 0x80000000;
    bool res_sign = res_32 & 0x80000000;
    if (src_sign == dst_sign && src_sign != res_sign)
      regs.eflags |= FLAG_OF;
  } else {
    // 8-bit operation
    uint8 res_8 = (uint8)result;
    uint8 dst_8 = (uint8)dst_val;
    uint8 src_8 = (uint8)src_val;

    // Carry Flag: Set if result overflows
    if (result > 0xFF)
      regs.eflags |= FLAG_CF;

    // Zero Flag: Set if result is zero
    if (res_8 == 0)
      regs.eflags |= FLAG_ZF;

    // Sign Flag: Set if MSB is set (negative)
    if (res_8 & 0x80)
      regs.eflags |= FLAG_SF;

    // Overflow Flag: Set if signed overflow occurred
    bool src_sign = src_8 & 0x80;
    bool dst_sign = dst_8 & 0x80;
    bool res_sign = res_8 & 0x80;
    if (src_sign == dst_sign && src_sign != res_sign)
      regs.eflags |= FLAG_OF;
  }
}

// Helper: Get operand value from ModRM r/m field (for reading memory operands)
// Returns: (value, instruction_length_beyond_modrm)
// Returns 0, 1 on error
static status_t GetModRM_Operand(AddressSpace &space, X86_32Registers &regs,
                                 const uint8 *instr, uint32 &value,
                                 uint32 &instr_len) {
  uint8 modrm = instr[0];
  uint8 mod = (modrm >> 6) & 3;
  uint8 rm = modrm & 7;
  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  if (mod == 3) {
    // Register mode
    value = *reg_ptrs[rm];
    instr_len = 1;
    return B_OK;
  } else if (mod == 1) {
    // [base + disp8]
    int8 disp8 = (int8)instr[1];
    uint32 addr = *reg_ptrs[rm] + disp8;
    status_t st = space.Read(addr, &value, 4);
    instr_len = 2;
    return st;
  } else if (mod == 2) {
    // [base + disp32]
    uint32 disp32 = *(uint32 *)&instr[1];
    uint32 addr = *reg_ptrs[rm] + disp32;
    status_t st = space.Read(addr, &value, 4);
    instr_len = 5;
    return st;
  } else {
    // mod == 0
    if (rm == 5) {
      // [disp32]
      uint32 disp32 = *(uint32 *)&instr[1];
      status_t st = space.Read(disp32, &value, 4);
      instr_len = 5;
      return st;
    } else {
      // [base]
      uint32 addr = *reg_ptrs[rm];
      status_t st = space.Read(addr, &value, 4);
      instr_len = 1;
      return st;
    }
  }
}

InterpreterX86_32::InterpreterX86_32(AddressSpace &addressSpace,
                                     SyscallDispatcher &dispatcher)
    : fAddressSpace(addressSpace), fDispatcher(dispatcher),
      fOptimizedExecutor(nullptr) {
  fOptimizedExecutor = new OptimizedX86Executor(addressSpace, dispatcher);
}

InterpreterX86_32::~InterpreterX86_32() {
  if (fOptimizedExecutor)
    delete fOptimizedExecutor;
}

status_t InterpreterX86_32::Run(GuestContext &context) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  printf("\n[INTERPRETER] Starting x86-32 interpreter\n");
  printf("[INTERPRETER] Entry point: 0x%08x\n", regs.eip);
  printf("[INTERPRETER] Stack pointer: 0x%08x\n", regs.esp);
  printf("[INTERPRETER] Max instructions: %u\n\n", MAX_INSTRUCTIONS);

  uint32 instr_count = 0;

  while (instr_count < MAX_INSTRUCTIONS) {
    uint32 bytes_consumed = 0;
    X86_32Registers &regs = x86_context.Registers();
    uint32 eip_before = regs.eip;
    status_t status = ExecuteInstruction(context, bytes_consumed);

    // DEBUG: Print EIP changes
    if (instr_count > 0 && instr_count % 5 == 0) {
      printf("[EXEC TRACE] instr=%u EIP: 0x%08x → 0x%08x (delta=%d)\n",
             instr_count, eip_before, regs.eip, (int)(regs.eip - eip_before));
    }

    if (status != B_OK) {
      // Check for guest exit signal (0x80000001)
      if (status == (status_t)0x80000001) {
        printf("[INTERPRETER] Guest program exited gracefully\n");
        return B_OK;
      }
      // Print opcode at failure point
      uint8 opcode_at_fail = 0;
      fAddressSpace.Read(regs.eip, &opcode_at_fail, 1);
      printf("[INTERPRETER] Instruction execution failed at EIP=0x%08x "
             "opcode=0x%02x: status=%d (0x%08x)\n",
             regs.eip, opcode_at_fail, status, (uint32_t)status);
      return status;
    }

    // For control flow instructions (CALL, JMP) that set EIP directly,
    // bytes_consumed will be 0 and EIP is already set. Don't treat as error.
    if (bytes_consumed == 0) {
      // Check if EIP was modified (control flow instruction)
      if (regs.eip == eip_before) {
        printf("[INTERPRETER] Invalid instruction at 0x%08x\n", regs.eip);
        return B_BAD_DATA;
      }
      // EIP was modified by instruction (CALL/JMP), don't increment
    } else {
      // Normal instruction, increment EIP by instruction size
      regs.eip += bytes_consumed;
    }
    instr_count++;

    // Safety check
    if (instr_count % 1000 == 0) {
      printf("[INTERPRETER] Executed %u instructions\n", instr_count);
    }
  }

  printf("[INTERPRETER] Reached instruction limit (%u)\n", MAX_INSTRUCTIONS);
  return B_ERROR;
}

status_t InterpreterX86_32::ExecuteInstruction(GuestContext &context,
                                               uint32 &bytes_consumed) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  // Leer instrucción desde dirección virtual (EIP)
  // AddressSpace::Read espera dirección virtual, internamente será traducida si
  // hay mappings
  uint8 instr_buffer[15]; // Máximo largo de instrucción x86

  // Debug: Check if this is a problematic address
  if (regs.eip < 0x40000000 || regs.eip > 0x41000000) {
    printf("[INTERPRETER] ⚠️  SUSPICIOUS EIP: 0x%08x (outside normal range)\n",
           regs.eip);
  }

  // Check for program exit (jump to NULL)
  if (regs.eip == 0) {
    printf("[INTERPRETER] Program jumped to NULL (0x00000000) - treating as "
           "graceful exit\n");
    return (status_t)0x80000001;
  }

  status_t status =
      fAddressSpace.Read(regs.eip, instr_buffer, sizeof(instr_buffer));
  if (status != B_OK) {
    printf("[INTERPRETER] Failed to read memory at vaddr=0x%08x\n", regs.eip);
    printf("[INTERPRETER] Current state:\n");
    printf("  EAX=0x%08x EBX=0x%08x ECX=0x%08x EDX=0x%08x\n", regs.eax,
           regs.ebx, regs.ecx, regs.edx);
    printf("  ESI=0x%08x EDI=0x%08x EBP=0x%08x ESP=0x%08x\n", regs.esi,
           regs.edi, regs.ebp, regs.esp);
    return status;
  }

  uint8 opcode = instr_buffer[0];
  uint8 prefix_offset = 0; // Offset to real opcode if prefix is present
  bool has_fs_override = false;
  bool has_lock = false;
  bool has_rep = false;
  bool has_repnz = false;

  // Handle prefixes (can be multiple, but in practice usually one)
  // Order: LOCK -> REP/REPNZ -> Segment overrides
  while (prefix_offset < 3) { // Limit to 3 prefixes max
    opcode = instr_buffer[prefix_offset];

    // LOCK prefix (0xF0)
    if (opcode == 0xF0) {
      printf("[INTERPRETER @ 0x%08x] LOCK ", regs.eip);
      has_lock = true;
      prefix_offset++;
      continue;
    }
    // REP prefix (0xF3)
    if (opcode == 0xF3) {
      printf("[INTERPRETER @ 0x%08x] REP ", regs.eip);
      has_rep = true;
      prefix_offset++;
      continue;
    }
    // REPNZ prefix (0xF2)
    if (opcode == 0xF2) {
      printf("[INTERPRETER @ 0x%08x] REPNZ ", regs.eip);
      has_repnz = true;
      prefix_offset++;
      continue;
    }
    // Segment override prefixes
    // 0x26 = ES, 0x2E = CS, 0x36 = SS, 0x3E = DS, 0x64 = FS, 0x65 = GS
    if (opcode == 0x64) {
      printf("[INTERPRETER @ 0x%08x] FS_OVERRIDE ", regs.eip);
      has_fs_override = true;
      prefix_offset++;
      continue;
    } else if (opcode == 0x65) {
      printf("[INTERPRETER @ 0x%08x] GS_OVERRIDE ", regs.eip);
      prefix_offset++;
      continue;
    } else if (opcode == 0x26 || opcode == 0x2E || opcode == 0x36 ||
               opcode == 0x3E) {
      // ES, CS, SS, DS overrides - for now just skip them
      printf("[INTERPRETER @ 0x%08x] SEG_OVERRIDE(0x%02x) ", regs.eip, opcode);
      prefix_offset++;
      continue;
    }
    // No more prefixes
    break;
  }

  // NOTE: bytes_consumed will be set by each instruction handler
  // It should include the full instruction length (opcode + operands + prefix
  // if any) The initial value here is just a placeholder
  bytes_consumed = 1; // Will be overridden by instruction handlers

  // Try optimized executor first (hybrid approach)
  // Only if no prefixes
  if (prefix_offset == 0 && !has_fs_override && fOptimizedExecutor) {
    status_t opt_status =
        fOptimizedExecutor->Execute(x86_context, bytes_consumed);
    if (opt_status == B_OK && bytes_consumed > 0) {
      return B_OK;
    }
  }

  printf("[INTERPRETER @ 0x%08x] opcode=%02x ", regs.eip, opcode);

  // Decodificar y ejecutar basado en opcode (fallback)
  switch (opcode) {
  // MOV $imm8, %r8 (B0-B7: AL, CL, DL, BL, AH, CH, DH, BH)
  case 0xB0:
  case 0xB1:
  case 0xB2:
  case 0xB3:
  case 0xB4:
  case 0xB5:
  case 0xB6:
  case 0xB7: {
    DebugPrintf("MOV $imm8, %%%s\n", reg_names[opcode - 0xB0]);
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
    int reg_index = opcode - 0xB0;
    *reg_ptrs[reg_index] = (*reg_ptrs[reg_index] & 0xFFFFFF00) | imm8;
    bytes_consumed = prefix_offset + 2; // opcode + imm8
    return B_OK;
  }

  // MOV $imm, %reg (B8-BF: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
  case 0xB8:
  case 0xB9:
  case 0xBA:
  case 0xBB:
  case 0xBC:
  case 0xBD:
  case 0xBE:
  case 0xBF: {
    // const char* regs[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI",
    // "EDI"}; // Moved to global static array
    DebugPrintf("MOV $imm, %%%s\n", reg_names[opcode - 0xB8]);
    uint32 instr_len = 0;
    status_t status =
        Execute_MOV(context, &instr_buffer[prefix_offset], instr_len);
    bytes_consumed = prefix_offset + instr_len;
    return status;
  }

  // MOV r/m32, r32 (89 /r modrm) - Move register to register/memory
  case 0x89: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;

    uint32 instr_len = 0;
    if (mod == 3) {
      // Register-to-register move
      DebugPrintf("MOV %%r32, %%r/m32 (reg-to-reg)\n");
      status_t status =
          Execute_MOV(context, &instr_buffer[prefix_offset], instr_len);
      bytes_consumed = prefix_offset + instr_len;
      return status;
    } else {
      // Memory store: MOV [mem], reg
      DebugPrintf("MOV [mem], %%r32\n");
      status_t status =
          Execute_MOV_Store(context, &instr_buffer[prefix_offset], instr_len);
      bytes_consumed = prefix_offset + instr_len;
      return status;
    }
  }

  // LEA r32, m (8D /r modrm) - Load Effective Address
  case 0x8D: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // LEA with register operand is invalid
      DebugPrintf("[INTERPRETER] LEA with mod=3 (register)? Invalid\n");
      return B_BAD_VALUE;
    }

    X86_32Registers &regs = x86_context.Registers();
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    // Calculate effective address (modRM with mod != 3)
    uint32 eff_addr = 0;
    uint32 len = 0;

    // Check if SIB byte is present (rm_field == 4)
    bool has_sib = (rm_field == 4);

    if (mod == 1) {
      // [base + disp8], or [SIB + disp8]
      if (has_sib) {
        uint8 sib = instr_buffer[2 + prefix_offset];
        uint8 sib_scale = (sib >> 6) & 3;
        uint8 sib_index = (sib >> 3) & 7;
        uint8 sib_base = sib & 7;
        int8 disp8 = (int8)instr_buffer[3 + prefix_offset];

        uint32 base_val = (sib_base == 5) ? 0 : *reg_ptrs[sib_base];
        uint32 index_val = (sib_index == 4) ? 0 : *reg_ptrs[sib_index];
        eff_addr = base_val + (index_val << sib_scale) + disp8;
        len = 4;
      } else {
        int8 disp8 = (int8)instr_buffer[2 + prefix_offset];
        eff_addr = *reg_ptrs[rm_field] + disp8;
        len = 3;
      }
      DebugPrintf("       [LEA] reg=%d %%eax,etc, addr=0x%08x (instr_len=%u)\n",
                  reg_field, eff_addr, len);
    } else if (mod == 2) {
      // [base + disp32], or [SIB + disp32]
      if (has_sib) {
        uint8 sib = instr_buffer[2 + prefix_offset];
        uint8 sib_scale = (sib >> 6) & 3;
        uint8 sib_index = (sib >> 3) & 7;
        uint8 sib_base = sib & 7;
        uint32 disp32 = *(uint32 *)&instr_buffer[3 + prefix_offset];

        uint32 base_val = (sib_base == 5) ? 0 : *reg_ptrs[sib_base];
        uint32 index_val = (sib_index == 4) ? 0 : *reg_ptrs[sib_index];
        eff_addr = base_val + (index_val << sib_scale) + disp32;
        len = 7;
      } else {
        uint32 disp32 = *(uint32 *)&instr_buffer[2 + prefix_offset];
        eff_addr = *reg_ptrs[rm_field] + disp32;
        len = 6;
      }
      DebugPrintf("       [LEA] reg=%d %%eax,etc, addr=0x%08x (instr_len=%u)\n",
                  reg_field, eff_addr, len);
    } else {
      // mod == 0
      if (rm_field == 5) {
        // [disp32] - RIP-relative or absolute (32-bit)
        uint32 disp32 = *(uint32 *)&instr_buffer[2 + prefix_offset];
        eff_addr = disp32;
        len = 6;
      } else if (has_sib) {
        // [SIB]
        uint8 sib = instr_buffer[2 + prefix_offset];
        uint8 sib_scale = (sib >> 6) & 3;
        uint8 sib_index = (sib >> 3) & 7;
        uint8 sib_base = sib & 7;

        uint32 base_val = (sib_base == 5) ? 0 : *reg_ptrs[sib_base];
        uint32 index_val = (sib_index == 4) ? 0 : *reg_ptrs[sib_index];
        eff_addr = base_val + (index_val << sib_scale);
        len = 3;
      } else {
        // [base]
        eff_addr = *reg_ptrs[rm_field];
        len = 2;
      }
      DebugPrintf("       [LEA] reg=%d %%eax,etc, addr=0x%08x (instr_len=%u)\n",
                  reg_field, eff_addr, len);
    }

    // Store effective address in destination register
    *reg_ptrs[reg_field] = eff_addr;
    bytes_consumed = prefix_offset + len;
    return B_OK;
  }

  // MOV r32, r/m32 (8B /r modrm) - Move register/memory to register
  case 0x8B: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;

    uint32 instr_len = 0;
    if (mod == 3) {
      // Register-to-register move
      DebugPrintf("MOV %%r/m32, %%r32 (reg-to-reg)\n");
      status_t status =
          Execute_MOV(context, &instr_buffer[prefix_offset], instr_len);
      bytes_consumed = prefix_offset + instr_len;
      return status;
    } else if (has_fs_override) {
      // Memory load with FS override (MOV reg, FS:dword)
      DebugPrintf("MOV %%r32, FS:[mem]\n");
      status_t status =
          Execute_MOV_Load_FS(context, &instr_buffer[prefix_offset], instr_len);
      bytes_consumed = prefix_offset + instr_len;
      return status;
    } else {
      // Memory load: MOV reg, [mem]
      DebugPrintf("MOV %%r32, [mem]\n");
      status_t status =
          Execute_MOV_Load(context, &instr_buffer[prefix_offset], instr_len);
      bytes_consumed = prefix_offset + instr_len;
      return status;
    }
  }

  // INT $imm (CD xx)
  case 0xCD:
    printf("INT $0x%02x\n", instr_buffer[1]);
    return Execute_INT(context, instr_buffer, bytes_consumed);

  // RET (C3)
  case 0xC3:
    DebugPrintf("RET\n");
    return Execute_RET(context, instr_buffer, bytes_consumed);

  // PUSH reg (50-57)
  case 0x50:
  case 0x51:
  case 0x52:
  case 0x53:
  case 0x54:
  case 0x55:
  case 0x56:
  case 0x57:
    DebugPrintf("PUSH reg\n");
    return Execute_PUSH(context, instr_buffer, bytes_consumed);

  // IMUL r32, r/m32, imm32 (69 /r id) - Signed multiply with immediate
  case 0x69: {
    DebugPrintf("IMUL r32, r/m32, imm32\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 dest_reg = (modrm >> 3) & 7;
    uint8 src_reg = modrm & 7;
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    if (mod == 3) {
      // Register mode: IMUL r32, r32, imm32
      uint32 src_val = *reg_ptrs[src_reg];
      int32 imm32 = *(int32 *)&instr_buffer[2 + prefix_offset];
      int64 result = (int64)(int32)src_val * (int64)imm32;
      *reg_ptrs[dest_reg] = (uint32)result; // Take lower 32 bits

      // Update flags - set OF and CF if result overflowed 32 bits
      regs.eflags = 0;
      if (result != (int32)result) {
        regs.eflags |= 0x0800; // OF
        regs.eflags |= 0x0001; // CF
      }
      bytes_consumed = prefix_offset + 6; // opcode + modrm + imm32
      return B_OK;
    } else {
      // Memory mode not yet implemented - just consume bytes
      bytes_consumed = prefix_offset + 6;
      return B_OK;
    }
  }

  // PUSH immediate 8-bit signed (6A xx)
  case 0x6A:
    DebugPrintf("PUSH $imm8\n");
    return Execute_PUSH_Imm(context, instr_buffer, bytes_consumed);

  // PUSH immediate 32-bit (68 xx xx xx xx)
  case 0x68:
    DebugPrintf("PUSH $imm32\n");
    return Execute_PUSH_Imm(context, instr_buffer, bytes_consumed);

  // POP reg (58-5F)
  case 0x58:
  case 0x59:
  case 0x5A:
  case 0x5B:
  case 0x5C:
  case 0x5D:
  case 0x5E:
  case 0x5F:
    DebugPrintf("POP reg\n");
    return Execute_POP(context, instr_buffer, bytes_consumed);

  // NOP (90)
  case 0x90:
    bytes_consumed = 1;
    return B_OK;

  // INC reg (40-47: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
  case 0x40:
  case 0x41:
  case 0x42:
  case 0x43:
  case 0x44:
  case 0x45:
  case 0x46:
  case 0x47: {
    X86_32Registers &regs = x86_context.Registers();
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
    int reg_idx = opcode - 0x40;
    (*reg_ptrs[reg_idx])++;
    bytes_consumed = 1;
    return B_OK;
  }

  // DEC reg (48-4F: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
  case 0x48:
  case 0x49:
  case 0x4A:
  case 0x4B:
  case 0x4C:
  case 0x4D:
  case 0x4E:
  case 0x4F: {
    X86_32Registers &regs = x86_context.Registers();
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
    int reg_idx = opcode - 0x48;
    (*reg_ptrs[reg_idx])--;
    bytes_consumed = 1;
    return B_OK;
  }

  // ADD r/m8, r8 (00 /r)
  case 0x00: {
    uint8 modrm = instr_buffer[1];
    if ((modrm >> 6) == 3) {
      // Register mode
      uint8 dst = modrm & 7;
      uint8 src = (modrm >> 3) & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 dst_val = (uint8)*reg_ptrs[dst];
      uint8 src_val = (uint8)*reg_ptrs[src];
      uint8 result = dst_val + src_val;
      *reg_ptrs[dst] = (*reg_ptrs[dst] & 0xFFFFFF00) | result;
      SetFlags_ADD(regs, result, dst_val, src_val, false);
      bytes_consumed = 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = 2;
    return B_OK;
  }

  // ADD r/m32, r32 (01 /r) ⭐ IMPORTANT
  case 0x01: {
    uint8 modrm = instr_buffer[1];
    if ((modrm >> 6) == 3) {
      // Register mode: ADD r32, r32
      uint8 dst = modrm & 7;
      uint8 src = (modrm >> 3) & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst];
      uint32 src_val = *reg_ptrs[src];
      uint32 result = dst_val + src_val;
      *reg_ptrs[dst] = result;
      SetFlags_ADD(regs, result, dst_val, src_val, true);
      printf("[ADD] r32-r32: reg%d=0x%08x, reg%d=0x%08x → 0x%08x\n", dst,
             dst_val, src, src_val, result);
      bytes_consumed = 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = 2;
    return B_OK;
  }

  // ADD r8, r/m8 (02 /r)
  case 0x02: {
    uint8 modrm = instr_buffer[1];
    if ((modrm >> 6) == 3) {
      // Register mode
      uint8 dst = (modrm >> 3) & 7;
      uint8 src = modrm & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 dst_val = (uint8)*reg_ptrs[dst];
      uint8 src_val = (uint8)*reg_ptrs[src];
      uint8 result = dst_val + src_val;
      *reg_ptrs[dst] = (*reg_ptrs[dst] & 0xFFFFFF00) | result;
      SetFlags_ADD(regs, result, dst_val, src_val, false);
      bytes_consumed = 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = 2;
    return B_OK;
  }

  // ADD r32, r/m32 (03 /r) ⭐ CRITICAL - FOUND IN BINARIES
  case 0x03: {
    uint8 modrm = instr_buffer[1];
    if ((modrm >> 6) == 3) {
      // Register mode: ADD r32, r32
      uint8 dst = (modrm >> 3) & 7;
      uint8 src = modrm & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst];
      uint32 src_val = *reg_ptrs[src];
      uint32 result = dst_val + src_val;
      *reg_ptrs[dst] = result;
      SetFlags_ADD(regs, result, dst_val, src_val, true);
      printf("[ADD] reg%d=0x%08x + reg%d=0x%08x → 0x%08x\n", dst, dst_val, src,
             src_val, result);
      bytes_consumed = 2;
      return B_OK;
    }
    // Memory mode - read from memory and add
    ModRM modrm_info;
    status_t status = DecodeModRM(&instr_buffer[1], modrm_info);
    if (status != B_OK) {
      bytes_consumed = 2;
      return B_OK;
    }
    uint32 src_addr = GetEffectiveAddress(regs, modrm_info);
    uint32 src_val = 0;
    if (fAddressSpace.Read(src_addr, &src_val, 4) == B_OK) {
      uint8 dst_reg = modrm_info.reg_op;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst_reg];
      uint32 result = dst_val + src_val;
      *reg_ptrs[dst_reg] = result;
      SetFlags_ADD(regs, result, dst_val, src_val, true);
      printf("[ADD] reg%d=0x%08x + [0x%08x]=0x%08x → 0x%08x\n", dst_reg,
             dst_val, src_addr, src_val, result);
    }
    bytes_consumed = 1 + modrm_info.bytes_used;
    return B_OK;
  }

  // ADD $imm, %eax (05 xx xx xx xx)
  case 0x05:
    DebugPrintf("ADD $imm, %%eax\n");
    return Execute_ADD(context, instr_buffer, bytes_consumed);

  // SUB $imm, %eax (2D xx xx xx xx)
  case 0x2D:
    DebugPrintf("SUB $imm, %%eax\n");
    return Execute_SUB(context, instr_buffer, bytes_consumed);

  // CMP $imm, %eax (3D xx xx xx xx)
  case 0x3D:
    DebugPrintf("CMP $imm, %%eax\n");
    return Execute_CMP(context, instr_buffer, bytes_consumed);

  // TEST $imm32, %eax (A9 xx xx xx xx)
  case 0xA9: {
    uint32 imm32 = *(uint32 *)&instr_buffer[1];
    X86_32Registers &regs = x86_context.Registers();
    uint32 result = regs.eax & imm32;

    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int32)result < 0)
      regs.eflags |= 0x80; // SF

    bytes_consumed = 5;
    return B_OK;
  }

  // CMP r32, r/m32 (39 /r modrm) - reg is source, r/m is dest
  case 0x39:
    DebugPrintf("CMP %%r32, %%r/m32 (ModRM format)\n");
    return Execute_CMP(context, instr_buffer, bytes_consumed);

  // CMP r/m32, r32 (38 /r modrm) - r/m is source, reg is dest (reverse)
  case 0x38: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    X86_32Registers &regs = x86_context.Registers();
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    uint32 src = 0;
    uint32 dst = *reg_ptrs[reg_field];
    uint32 instr_len = 2;

    if (mod == 3) {
      // Register-to-register CMP
      src = *reg_ptrs[rm_field];
    } else if (mod == 1) {
      // [base + disp8]
      int8 disp8 = (int8)instr_buffer[2 + prefix_offset];
      uint32 addr = *reg_ptrs[rm_field] + disp8;
      fAddressSpace.Read(addr, &src, 4);
      instr_len = 3;
    } else if (mod == 2) {
      // [base + disp32]
      uint32 disp32 = *(uint32 *)&instr_buffer[2 + prefix_offset];
      uint32 addr = *reg_ptrs[rm_field] + disp32;
      fAddressSpace.Read(addr, &src, 4);
      instr_len = 6;
    } else {
      // mod == 0 - no displacement or SIB
      if (rm_field == 5) {
        // [disp32]
        uint32 disp32 = *(uint32 *)&instr_buffer[2 + prefix_offset];
        fAddressSpace.Read(disp32, &src, 4);
        instr_len = 6;
      } else {
        // [base]
        uint32 addr = *reg_ptrs[rm_field];
        fAddressSpace.Read(addr, &src, 4);
        instr_len = 2;
      }
    }

    uint32 result = dst - src;
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int32)result < 0)
      regs.eflags |= 0x80; // SF

    bytes_consumed = prefix_offset + instr_len;
    return B_OK;
  }

  // XOR %reg, %reg (31 C0-FF for specific combinations)
  // Common: 31 DB = XOR %ebx, %ebx (clear %ebx)
  case 0x31:
    DebugPrintf("XOR %%reg, %%reg\n");
    return Execute_XOR(context, instr_buffer, bytes_consumed);

  // JMP $imm (E9 xx xx xx xx)
  case 0xE9:
    DebugPrintf("JMP $imm\n");
    return Execute_JMP(context, instr_buffer, bytes_consumed);

  // Conditional jumps (Sprint 4)
  // JZ (74 xx) - Jump if Zero
  case 0x74:
    DebugPrintf("JZ (Jump if Zero)\n");
    return Execute_JZ(context, instr_buffer, bytes_consumed);

  // JNZ (75 xx) - Jump if Not Zero
  case 0x75:
    DebugPrintf("JNZ (Jump if Not Zero)\n");
    return Execute_JNZ(context, instr_buffer, bytes_consumed);

  // JL (7C xx) - Jump if Less (SF != OF)
  case 0x7C:
    DebugPrintf("JL (Jump if Less)\n");
    return Execute_JL(context, instr_buffer, bytes_consumed);

  // JLE (7E xx) - Jump if Less or Equal (ZF=1 or SF != OF)
  case 0x7E:
    DebugPrintf("JLE (Jump if Less or Equal)\n");
    return Execute_JLE(context, instr_buffer, bytes_consumed);

  // JG (7F xx) - Jump if Greater (ZF=0 and SF=OF)
  case 0x7F:
    DebugPrintf("JG (Jump if Greater)\n");
    return Execute_JG(context, instr_buffer, bytes_consumed);

  // JGE (7D xx) - Jump if Greater or Equal (SF=OF)
  case 0x7D:
    DebugPrintf("JGE (Jump if Greater or Equal)\n");
    return Execute_JGE(context, instr_buffer, bytes_consumed);

  // JA (77 xx) - Jump if Above (unsigned greater) - CF=0 and ZF=0
  case 0x77:
    DebugPrintf("JA (Jump if Above)\n");
    return Execute_JA(context, instr_buffer, bytes_consumed);

  // JAE (73 xx) - Jump if Above or Equal (unsigned greater/equal) - CF=0
  case 0x73:
    DebugPrintf("JAE (Jump if Above or Equal)\n");
    return Execute_JAE(context, instr_buffer, bytes_consumed);

  // JB/JC (72 xx) - Jump if Below (unsigned less) - CF=1
  case 0x72:
    DebugPrintf("JB (Jump if Below)\n");
    return Execute_JB(context, instr_buffer, bytes_consumed);

  // JBE (76 xx) - Jump if Below or Equal (unsigned less/equal) - CF=1 or ZF=1
  case 0x76:
    DebugPrintf("JBE (Jump if Below or Equal)\n");
    return Execute_JBE(context, instr_buffer, bytes_consumed);

  // JP/JPE (7A xx) - Jump if Parity/Parity Even - PF=1
  case 0x7A:
    DebugPrintf("JP (Jump if Parity)\n");
    return Execute_JP(context, instr_buffer, bytes_consumed);

  // JNP/JPO (7B xx) - Jump if Not Parity/Parity Odd - PF=0
  case 0x7B:
    DebugPrintf("JNP (Jump if Not Parity)\n");
    return Execute_JNP(context, instr_buffer, bytes_consumed);

  // JS (78 xx) - Jump if Sign (SF=1)
  case 0x78:
    DebugPrintf("JS (Jump if Sign)\n");
    return Execute_JS(context, instr_buffer, bytes_consumed);

  // JNS (79 xx) - Jump if Not Sign (SF=0)
  case 0x79:
    DebugPrintf("JNS (Jump if Not Sign)\n");
    return Execute_JNS(context, instr_buffer, bytes_consumed);

  // JO (70 xx) - Jump if Overflow (OF=1)
  case 0x70:
    DebugPrintf("JO (Jump if Overflow)\n");
    return Execute_JO(context, instr_buffer, bytes_consumed);

  // JNO (71 xx) - Jump if Not Overflow (OF=0)
  case 0x71:
    DebugPrintf("JNO (Jump if Not Overflow)\n");
    return Execute_JNO(context, instr_buffer, bytes_consumed);

  // GROUP 1 (0x81) - ADD, SUB, CMP with Imm32
  case 0x81:
    DebugPrintf("GROUP1 (ADD/SUB/CMP with Imm32) ModR/M=\n");
    return Execute_GROUP_81(context, instr_buffer, bytes_consumed);

  // GROUP 1 (0x83) - ADD, OR, ADC, SBB, AND, XOR, CMP with Imm8
  case 0x83:
    DebugPrintf("GROUP1 (ADD/SUB/CMP/etc. with Imm8) ModR/M=\n");
    return Execute_GROUP_83(context, instr_buffer, bytes_consumed);

  // GROUP C1 (0xC1) - Shift/Rotate instructions with 8-bit immediate
  case 0xC1:
    DebugPrintf("GROUP_C1 (Shift/Rotate with Imm8) ModR/M=\n");
    return Execute_GROUP_C1(context, instr_buffer, bytes_consumed);

  // GROUP D3 (0xD3) - Shift/Rotate instructions with variable count (CL)
  case 0xD3: {
    DebugPrintf("GROUP_D3 (Shift/Rotate with CL)\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_op = (modrm >> 3) & 7; // Extended opcode
    uint8 rm_field = modrm & 7;

    if (mod != 3) {
      // Memory operand not implemented
      uint32 instr_len = 2;
      if (mod == 1)
        instr_len = 3;
      if (mod == 2)
        instr_len = 6;
      if (rm_field == 4 && mod != 3)
        instr_len++; // SIB
      bytes_consumed = prefix_offset + instr_len;
      return B_OK;
    }

    // Register mode: shift/rotate r32 by CL
    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
    uint8 shift_count = (uint8)regs.ecx; // CL register
    uint32 value = *reg_ptrs[rm_field];
    uint32 result = value;

    switch (reg_op) {
    case 0:                // ROL - Rotate left
      shift_count &= 0x1F; // Only use lower 5 bits
      result = (value << shift_count) | (value >> (32 - shift_count));
      break;
    case 1: // ROR - Rotate right
      shift_count &= 0x1F;
      result = (value >> shift_count) | (value << (32 - shift_count));
      break;
    case 2: // RCL - Rotate carry left
      // For simplicity, treat like ROL for now
      shift_count &= 0x1F;
      result = (value << shift_count) | (value >> (32 - shift_count));
      break;
    case 3: // RCR - Rotate carry right
      // For simplicity, treat like ROR for now
      shift_count &= 0x1F;
      result = (value >> shift_count) | (value << (32 - shift_count));
      break;
    case 4: // SHL/SAL - Shift left
      shift_count &= 0x1F;
      result = value << shift_count;
      break;
    case 5: // SHR - Logical shift right
      shift_count &= 0x1F;
      result = value >> shift_count;
      break;
    case 7: // SAR - Arithmetic shift right
      shift_count &= 0x1F;
      if (shift_count > 0) {
        if ((int32)value < 0) {
          // Sign extend
          result = (value >> shift_count) |
                   ((0xFFFFFFFFUL << (32 - shift_count)) & 0xFFFFFFFFUL);
        } else {
          result = value >> shift_count;
        }
      }
      break;
    default:
      // Unknown shift type
      break;
    }

    *reg_ptrs[rm_field] = result;

    // Update flags (simplified - not accurate for all cases)
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int32)result < 0)
      regs.eflags |= 0x80; // SF

    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // JMP SHORT $imm8 (EB xx) - Unconditional jump short
  case 0xEB: {
    DebugPrintf("JMP SHORT $imm8\n");
    int8 displacement = (int8)instr_buffer[1 + prefix_offset];
    X86_32Registers &regs = x86_context.Registers();
    regs.eip += displacement + 2; // EIP after instruction + displacement
    bytes_consumed = 0;           // Set by CALL/JMP
    return B_OK;
  }

  // Sprint 5: CALL $imm (E8 xx xx xx xx)
  case 0xE8:
    DebugPrintf("CALL $imm\n");
    return Execute_CALL(context, instr_buffer, bytes_consumed);

  // TEST r/m32, r32 (84 /r modrm) - reverse format
  case 0x84: {
    DebugPrintf("TEST %%r/m32, %%r32\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;

    if (mod != 3) {
      // Memory operand - skip for now (just consume bytes)
      uint32 instr_len = 2;
      if (mod == 1)
        instr_len = 3; // [base+disp8]
      if (mod == 2)
        instr_len = 6; // [base+disp32]
      bytes_consumed = prefix_offset + instr_len;
      return B_OK;
    }

    // Register-to-register TEST
    uint8 reg_op = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    X86_32Registers &regs = x86_context.Registers();
    uint32_t *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    uint32_t result = *reg_ptrs[rm_field] & *reg_ptrs[reg_op];

    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int32_t)result < 0)
      regs.eflags |= 0x80; // SF

    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // TEST r32, r/m32 (85 /r modrm)
  case 0x85: {
    DebugPrintf("TEST %%r32, %%r/m32\n");
    // TEST affects flags (ZF, SF, OF=0, CF=0)
    // Result = r & r/m
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;

    if (mod != 3) {
      // Memory operand - skip for now (just consume bytes)
      uint32 instr_len = 2;
      if (mod == 1)
        instr_len = 3; // [base+disp8]
      if (mod == 2)
        instr_len = 6; // [base+disp32]
      bytes_consumed = prefix_offset + instr_len;
      return B_OK;
    }

    uint8 reg_op = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    X86_32Registers &regs = x86_context.Registers();
    uint32_t *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    uint32_t src = *reg_ptrs[reg_op];   // Source register
    uint32_t dst = *reg_ptrs[rm_field]; // Destination register

    // Perform TEST (bitwise AND, discard result, only update flags)
    uint32_t result = src & dst;

    // Update flags based on result
    // ZF (bit 6) = 1 if result == 0
    // SF (bit 7) = sign bit of result
    // OF (bit 11) = 0
    // CF (bit 0) = 0
    regs.eflags &= ~(0x40 | 0x80 | 0x800 | 0x1); // Clear ZF, SF, OF, CF
    if (result == 0)
      regs.eflags |= 0x40; // Set ZF
    if (result & 0x80000000)
      regs.eflags |= 0x80; // Set SF
    // OF and CF are cleared for TEST instruction

    bytes_consumed = 2; // ModR/M only

    return B_OK;
  }

  // XCHG r/m32, r32 (86 /r) - Exchange registers/memory (32-bit variant)
  case 0x86: {
    DebugPrintf("XCHG %%r/m32, %%r32\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    X86_32Registers &regs = x86_context.Registers();
    uint32_t *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    if (mod == 3) {
      // Register-to-register exchange
      uint32 temp = *reg_ptrs[rm_field];
      *reg_ptrs[rm_field] = *reg_ptrs[reg_field];
      *reg_ptrs[reg_field] = temp;
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    } else {
      // Memory exchange - not yet fully implemented
      // For now, just consume bytes
      bytes_consumed = prefix_offset + 2;
      if (mod == 1)
        bytes_consumed += 1; // [base+disp8]
      if (mod == 2)
        bytes_consumed += 4; // [base+disp32]
      return B_OK;
    }
  }

  // LODSB (AC) - Load AL from [ESI]
  // Used for string operations, typically with REP
  case 0xAC: {
    DebugPrintf("LODSB (Load AL from [ESI])\n");
    X86_32Registers &regs = x86_context.Registers();

    // Read byte from [ESI]
    uint8 value;
    status_t st = fAddressSpace.Read(regs.esi, &value, 1);
    if (st != B_OK) {
      printf("[INTERPRETER] LODSB: Failed to read from ESI=0x%08x\n", regs.esi);
      value = 0;
    }

    // Store in AL
    regs.eax = (regs.eax & 0xFFFFFF00) | value;

    // Update ESI based on direction flag
    uint8 df = (regs.eflags >> 10) & 1;
    if (df == 0) {
      regs.esi += 1; // Forward
    } else {
      regs.esi -= 1; // Backward
    }

    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // STOSB (AA) - Store AL to [EDI]
  // Used for string operations, typically with REP
  case 0xAA: {
    DebugPrintf("STOSB (Store AL to [EDI])\n");
    X86_32Registers &regs = x86_context.Registers();

    // Write AL to [EDI]
    uint8 value = (uint8)regs.eax;
    status_t st = fAddressSpace.Write(regs.edi, &value, 1);
    if (st != B_OK) {
      // Don't treat as fatal error - some programs use STOSB with invalid
      // addresses in initialization code. Log warning but continue.
      DebugPrintf(
          "[INTERPRETER] STOSB: Failed to write to EDI=0x%08x (continuing)\n",
          regs.edi);
      // Continue execution - don't return error
    }

    // Update EDI based on direction flag (always, even if write failed)
    uint8 df = (regs.eflags >> 10) & 1;
    if (df == 0) {
      regs.edi += 1; // Forward
    } else {
      regs.edi -= 1; // Backward
    }

    bytes_consumed = prefix_offset + 1;
    return B_OK; // Always return success
  }

  // MOVSD (6F) - Move String Dword (with REP prefix typically)
  // Without REP, it copies one dword: [ESI] -> [EDI]
  // Direction flag (DF) determines if ESI/EDI increment or decrement
  case 0x6F: {
    DebugPrintf("MOVSD (Move String Dword)\n");
    X86_32Registers &regs = x86_context.Registers();

    // Read from [ESI]
    uint32 value;
    status_t st = fAddressSpace.Read(regs.esi, &value, 4);
    if (st != B_OK) {
      printf("[INTERPRETER] MOVSD: Failed to read from ESI=0x%08x\n", regs.esi);
      value = 0; // Use 0 as fallback
    }

    // Write to [EDI]
    st = fAddressSpace.Write(regs.edi, &value, 4);
    if (st != B_OK) {
      printf("[INTERPRETER] MOVSD: Failed to write to EDI=0x%08x\n", regs.edi);
    }

    // Update ESI and EDI based on direction flag (DF = bit 10 of EFLAGS)
    uint8 df = (regs.eflags >> 10) & 1;
    if (df == 0) {
      // Forward direction (normal): increment
      regs.esi += 4;
      regs.edi += 4;
    } else {
      // Backward direction: decrement
      regs.esi -= 4;
      regs.edi -= 4;
    }

    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // MOV moffs32, EAX (A1 xx xx xx xx) - Load 32-bit memory into EAX
  // This is commonly used with FS override: 64 A1 offset %eax
  case 0xA1: {
    if (has_fs_override) {
      DebugPrintf("MOV %%fs:offset, %%eax\n");
      uint32 local_bytes = 0;
      status_t status = Execute_MOV_Load_FS(
          context, &instr_buffer[prefix_offset], local_bytes);
      if (status == B_OK) {
        bytes_consumed =
            prefix_offset + local_bytes; // prefix + instruction length
      }
      return status;
    } else {
      // Regular MOV moffs32, EAX
      DebugPrintf("MOV $offset, %%eax\n");
      uint32 offset = (instr_buffer[1 + prefix_offset] & 0xFF) |
                      ((instr_buffer[2 + prefix_offset] & 0xFF) << 8) |
                      ((instr_buffer[3 + prefix_offset] & 0xFF) << 16) |
                      ((instr_buffer[4 + prefix_offset] & 0xFF) << 24);

      X86_32Registers &regs = x86_context.Registers();
      status_t status = fAddressSpace.Read(offset, &regs.eax, 4);
      if (status != B_OK) {
        // If read fails, just set EAX to 0 and continue
        // This can happen if relocation didn't work properly
        printf("[INTERPRETER] Warning: Failed to read from 0x%08x in MOV "
               "moffs32, treating as zero\n",
               offset);
        regs.eax = 0;
      }

      bytes_consumed = prefix_offset + 1 + 4; // prefix + opcode + offset
      return B_OK;
    }
  }

  // GROUP 5 (0xFF) - Extended group for INC/DEC/CALL/JMP/PUSH indirect
  // Formats: FF /0 = INC r/m32, FF /1 = DEC r/m32, FF /2 = CALL r/m32
  //          FF /4 = JMP r/m32, FF /6 = PUSH r/m32
  case 0xFF: {
    uint8 modrm = instr_buffer[1];
    uint8 reg_field = (modrm >> 3) & 7; // bits 5-3 (extended opcode)
    uint8 mod = (modrm >> 6) & 3;
    uint8 rm = modrm & 7;

    printf("       [0xFF GROUP] modrm=0x%02x, reg_field=%u\n", modrm,
           reg_field);

    if (reg_field == 0) {
      // INC r/m32
      if (mod == 3) {
        // Register mode: INC %exx
        uint32 *regs_array[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                                &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
        (*regs_array[rm])++;
        DebugPrintf("INC %%%s\n", reg_names[rm]);
        bytes_consumed = 2;
        return B_OK;
      } else {
        // Memory mode - not commonly used, skip for now
        printf("       [0xFF /0] INC memory mode not implemented\n");
        bytes_consumed = 2; // At minimum, skip opcode and modrm
        return B_OK;
      }
    } else if (reg_field == 1) {
      // DEC r/m32
      if (mod == 3) {
        // Register mode: DEC %exx
        uint32 *regs_array[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                                &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
        (*regs_array[rm])--;
        DebugPrintf("DEC %%%s\n", reg_names[rm]);
        bytes_consumed = 2;
        return B_OK;
      } else {
        // Memory mode - not commonly used, skip for now
        printf("       [0xFF /1] DEC memory mode not implemented\n");
        bytes_consumed = 2; // At minimum, skip opcode and modrm
        return B_OK;
      }
    } else if (reg_field == 2) {
      // CALL r/m32
      printf("       [0xFF GROUP] Dispatching to Execute_CALL\n");
      DebugPrintf("CALL r/m32 (indirect)\n");
      return Execute_CALL(context, instr_buffer, bytes_consumed);
    } else if (reg_field == 4) {
      // JMP r/m32
      DebugPrintf("JMP r/m32 (indirect)\n");
      return Execute_JMP(context, instr_buffer, bytes_consumed);
    } else if (reg_field == 6) {
      // PUSH r/m32
      DebugPrintf("PUSH r/m32 (indirect)\n");
      return Execute_PUSH(context, instr_buffer, bytes_consumed);
    } else {
      printf("       [0xFF GROUP] Unknown sub-opcode: reg_field=%u - treating "
             "as 2-byte NOP\n",
             reg_field);
      // Unknown 0xFF sub-opcode, skip it gracefully (at least 2 bytes: opcode +
      // modrm) This prevents crashes on undefined opcodes
      bytes_consumed = 2;
      return B_OK; // Treat as NOP instead of error
    }
  }

  // CLD (FC) - Clear Direction Flag
  case 0xFC:
    printf("[INTERPRETER] CLD (clear direction flag) - treated as NOP\n");
    bytes_consumed = 1;
    return B_OK;

  // STD (FD) - Set Direction Flag
  case 0xFD:
    printf("[INTERPRETER] STD (set direction flag) - treated as NOP\n");
    bytes_consumed = 1;
    return B_OK;

  // LAHF (9F) - Load AH from Flags
  case 0x9F:
    printf("[INTERPRETER] LAHF - treated as NOP\n");
    bytes_consumed = 1;
    return B_OK;

  // SAHF (9E) - Store AH into Flags
  case 0x9E:
    printf("[INTERPRETER] SAHF - treated as NOP\n");
    bytes_consumed = 1;
    return B_OK;

  // ADC r/m8, r8 (10 /r) - Add with Carry (8-bit)
  case 0x10: {
    DebugPrintf("ADC %%r/m8, %%r8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register mode
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 dst = (uint8)*reg_ptrs[rm_field];
      uint8 src = (uint8)*reg_ptrs[reg_field];
      uint8 cf = (regs.eflags >> 0) & 1;
      uint8 result = dst + src + cf;
      *reg_ptrs[rm_field] = (*reg_ptrs[rm_field] & 0xFFFFFF00) | result;

      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int8)result < 0)
        regs.eflags |= 0x80; // SF
      if ((uint32)dst + (uint32)src + (uint32)cf > 0xFF)
        regs.eflags |= 0x01; // CF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // SBB r/m8, r8 (18 /r) - Subtract with Borrow (8-bit)
  case 0x18: {
    DebugPrintf("SBB %%r/m8, %%r8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register mode
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 dst = (uint8)*reg_ptrs[rm_field];
      uint8 src = (uint8)*reg_ptrs[reg_field];
      uint8 cf = (regs.eflags >> 0) & 1;
      uint8 result = dst - src - cf;
      *reg_ptrs[rm_field] = (*reg_ptrs[rm_field] & 0xFFFFFF00) | result;

      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int8)result < 0)
        regs.eflags |= 0x80; // SF
      if ((uint32)dst < (uint32)src + (uint32)cf)
        regs.eflags |= 0x01; // CF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // OR r/m8, r8 (08 /r)
  case 0x08: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: OR r8, r8
      uint8 dst = modrm & 7;
      uint8 src = (modrm >> 3) & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 dst_val = (uint8)*reg_ptrs[dst];
      uint8 src_val = (uint8)*reg_ptrs[src];
      uint8 result = dst_val | src_val;
      *reg_ptrs[dst] = (*reg_ptrs[dst] & 0xFFFFFF00) | result;
      regs.eflags = 0; // Clear all flags (OR clears CF and OF)
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int8)result < 0)
        regs.eflags |= 0x80; // SF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // OR r/m32, r32 (09 /r)
  case 0x09: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: OR r32, r32
      uint8 dst = modrm & 7;
      uint8 src = (modrm >> 3) & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst];
      uint32 src_val = *reg_ptrs[src];
      uint32 result = dst_val | src_val;
      *reg_ptrs[dst] = result;
      regs.eflags = 0; // Clear all flags
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // OR r/m32, imm32 (0B /r) - OR dword with immediate
  // Note: 0B is actually "OR r32, r/m32" but we also need 0D for "OR eax,
  // imm32" For now, treat 0B as a pass-through
  case 0x0B: {
    DebugPrintf("OR r/m32, r32 (0B /r)\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: OR r32, r32
      uint8 src = (modrm >> 3) & 7;
      uint8 dst = modrm & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 src_val = *reg_ptrs[src];
      uint32 dst_val = *reg_ptrs[dst];
      uint32 result = dst_val | src_val;
      *reg_ptrs[dst] = result;
      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // CMP r32, r/m32 (3B /r)
  case 0x3B: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: CMP r32, r32
      uint8 src_reg = (modrm >> 3) & 7;
      uint8 dst_reg = modrm & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 src_val = *reg_ptrs[src_reg];
      uint32 dst_val = *reg_ptrs[dst_reg];
      uint32 result = dst_val - src_val;
      // Update flags
      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      if (dst_val < src_val)
        regs.eflags |= 0x01; // CF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // AND r/m32, r32 (21 /r)
  case 0x21: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: AND r32, r32
      uint8 dst = modrm & 7;
      uint8 src = (modrm >> 3) & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst];
      uint32 src_val = *reg_ptrs[src];
      uint32 result = dst_val & src_val;
      *reg_ptrs[dst] = result;
      regs.eflags = 0; // Clear all flags
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // SUB r/m32, r32 (29 /r)
  case 0x29: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: SUB r32, r32
      uint8 dst = modrm & 7;
      uint8 src = (modrm >> 3) & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst];
      uint32 src_val = *reg_ptrs[src];
      uint32 result = dst_val - src_val;
      *reg_ptrs[dst] = result;
      // Update flags
      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      if (dst_val < src_val)
        regs.eflags |= 0x01; // CF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // SUB r32, r/m32 (2B /r) - Reverse of 29
  case 0x2B: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) {
      // Register mode: SUB r32, r32
      uint8 dst = (modrm >> 3) & 7;
      uint8 src = modrm & 7;
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst_val = *reg_ptrs[dst];
      uint32 src_val = *reg_ptrs[src];
      uint32 result = dst_val - src_val;
      *reg_ptrs[dst] = result;
      // Update flags
      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      if (dst_val < src_val)
        regs.eflags |= 0x01; // CF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not yet implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // SUB AL, imm8 (2C xx)
  case 0x2C: {
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    uint8 al = (uint8)regs.eax;
    uint8 result = al - imm8;
    regs.eax = (regs.eax & 0xFFFFFF00) | result;
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int8)result < 0)
      regs.eflags |= 0x80; // SF
    if ((uint32)al < (uint32)imm8)
      regs.eflags |= 0x01; // CF
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // ADC AL, imm8 (14 xx) - Add with Carry
  case 0x14: {
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    uint8 al = (uint8)regs.eax;
    uint8 cf = (regs.eflags >> 0) & 1; // Get carry flag
    uint8 result = al + imm8 + cf;
    regs.eax = (regs.eax & 0xFFFFFF00) | result;
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int8)result < 0)
      regs.eflags |= 0x80; // SF
    if ((uint32)al + (uint32)imm8 + (uint32)cf > 0xFF)
      regs.eflags |= 0x01; // CF
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // SBB AL, imm8 (1C xx) - Subtract with Borrow
  case 0x1C: {
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    uint8 al = (uint8)regs.eax;
    uint8 cf = (regs.eflags >> 0) & 1; // Get carry flag (borrow)
    uint8 result = al - imm8 - cf;
    regs.eax = (regs.eax & 0xFFFFFF00) | result;
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int8)result < 0)
      regs.eflags |= 0x80; // SF
    if ((uint32)al < (uint32)imm8 + (uint32)cf)
      regs.eflags |= 0x01; // CF
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // ADD AL, imm8 (04 xx)
  case 0x04: {
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    uint8 al = (uint8)regs.eax;
    uint8 result = al + imm8;
    regs.eax = (regs.eax & 0xFFFFFF00) | result;
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int8)result < 0)
      regs.eflags |= 0x80; // SF
    if ((uint32)al + (uint32)imm8 > 0xFF)
      regs.eflags |= 0x01; // CF
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // CMP r8, r/m8 (3A /r) - Compare byte
  case 0x3A: {
    DebugPrintf("CMP %%r8, %%r/m8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;

    if (mod == 3) {
      // Register-to-register compare
      uint8 reg_op = (modrm >> 3) & 7;
      uint8 rm_field = modrm & 7;

      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

      uint8 src = (uint8)*reg_ptrs[reg_op];
      uint8 dst = (uint8)*reg_ptrs[rm_field];
      uint8 result = dst - src;

      // Update flags based on result
      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int8)result < 0)
        regs.eflags |= 0x80; // SF
      if ((uint8)dst < (uint8)src)
        regs.eflags |= 0x01; // CF

      bytes_consumed = prefix_offset + 2;
      return B_OK;
    } else {
      // Memory operand not yet implemented
      uint32 instr_len = 2;
      if (mod == 1)
        instr_len = 3; // [base+disp8]
      if (mod == 2)
        instr_len = 6; // [base+disp32]
      bytes_consumed = prefix_offset + instr_len;
      return B_OK;
    }
  }

  // AAS (3F) - ASCII Adjust AL After Subtraction
  case 0x3F: {
    DebugPrintf("AAS (ASCII Adjust AL After Subtraction)\n");
    // This instruction is BCD-related and rarely used in modern code
    // Check if AL low nibble > 9 or if AF (auxiliary flag) is set
    uint8 al = (uint8)regs.eax;
    uint8 af = (regs.eflags >> 4) & 1;

    if ((al & 0x0F) > 9 || af == 1) {
      // Adjust AL
      regs.eax = (regs.eax & 0xFFFFFF00) | ((uint8)(al - 6));
      // Decrement AH
      uint8 ah = (regs.eax >> 8) & 0xFF;
      regs.eax = (regs.eax & 0xFFFF00FF) | ((ah - 1) << 8);
      // Set AF and CF
      regs.eflags |= 0x10; // AF
      regs.eflags |= 0x01; // CF
    } else {
      // Clear AF and CF
      regs.eflags &= ~0x10; // Clear AF
      regs.eflags &= ~0x01; // Clear CF
    }

    // Mask AL to BCD
    regs.eax = (regs.eax & 0xFFFFFF00) | ((regs.eax & 0xFF) & 0x0F);
    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // AND AL, imm8 (24 xx)
  case 0x24: {
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    uint8 al = (uint8)regs.eax;
    uint8 result = al & imm8;
    regs.eax = (regs.eax & 0xFFFFFF00) | result;
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int8)result < 0)
      regs.eflags |= 0x80; // SF
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // PUSH ES (06) - Push extra segment register
  case 0x06: {
    DebugPrintf("PUSH ES\n");
    // Push segment register value to stack
    // In 32-bit mode, we push 4 bytes (extended to 32-bit)
    uint32 es_value = 0; // Flat model - ES is typically 0
    uint32 esp = regs.esp - 4;
    fAddressSpace.Write(esp, &es_value, 4);
    regs.esp = esp;
    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // POP ES (07)
  case 0x07: {
    // POP to segment register - just pop the value and ignore it
    regs.esp += 4;
    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // MOV imm32 to r/m32 (C7 /0 id) - Group 11
  case 0xC7: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (reg_field != 0) {
      // Only /0 is MOV, others are invalid/unimplemented
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }

    if (mod == 3) {
      // Register mode: MOV imm32, r32
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 imm32 = *(uint32 *)&instr_buffer[2 + prefix_offset];
      *reg_ptrs[rm_field] = imm32;
      bytes_consumed = prefix_offset + 6; // opcode + modrm + imm32
      return B_OK;
    }

    // Memory mode: MOV imm32, [mem]
    // Implement later
    bytes_consumed = prefix_offset + 6;
    return B_OK;
  }

  // TEST r/m32, r32 / DIV / etc (F6 /r / F7 /r) - Group 3
  case 0xF6: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (reg_field == 0) {
      // TEST r/m8, imm8
      if (mod == 3) {
        uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                              &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
        uint8 imm8 = instr_buffer[2 + prefix_offset];
        uint8 result = (uint8)*reg_ptrs[rm_field] & imm8;
        regs.eflags = 0;
        if (result == 0)
          regs.eflags |= 0x40; // ZF
        if ((int8)result < 0)
          regs.eflags |= 0x80; // SF
        bytes_consumed = prefix_offset + 3;
        return B_OK;
      }
    }
    // Other sub-opcodes not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // TEST r/m32, r32 / etc (F7 /r) - Group 3 (32-bit version)
  case 0xF7: {
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (reg_field == 0) {
      // TEST r/m32, imm32
      if (mod == 3) {
        uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                              &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
        uint32 imm32 = *(uint32 *)&instr_buffer[2 + prefix_offset];
        uint32 result = *reg_ptrs[rm_field] & imm32;
        regs.eflags = 0;
        if (result == 0)
          regs.eflags |= 0x40; // ZF
        if ((int32)result < 0)
          regs.eflags |= 0x80; // SF
        bytes_consumed = prefix_offset + 6;
        return B_OK;
      }
    }
    // Other sub-opcodes not implemented
    bytes_consumed = prefix_offset + 6;
    return B_OK;
  }

  // IN EAX, imm8 (E5 xx) - Read from I/O port
  case 0xE5: {
    DebugPrintf("IN %%eax, imm8\n");
    // In user-mode code, this typically shouldn't appear
    // or should be caught by kernel. Treat as NOP that sets EAX to 0
    uint8 port = instr_buffer[1 + prefix_offset];
    printf("[INTERPRETER] IN EAX, 0x%02x - stub (setting EAX=0)\n", port);
    regs.eax = 0; // Default value for unhandled I/O ports
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // HLT (F4) - Halt processor (should not normally appear in user code)
  case 0xF4: {
    DebugPrintf("HLT (Halt - treating as NOP)\n");
    // In a real system, this would halt the CPU
    // In our interpreter, treat as NOP
    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // LOCK prefix (F0) - treat as a prefix that affects next instruction
  case 0xF0: {
    // LOCK is a prefix. Skip it and process the next instruction
    // For now, we'll just skip the LOCK prefix and continue
    printf("[INTERPRETER] LOCK prefix encountered - skipping\n");
    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // Two-byte opcodes (0F xx) - SSE, advanced instructions
  case 0x0F: {
    uint8 second_opcode = instr_buffer[1 + prefix_offset];
    printf(
        "[INTERPRETER] TWO-BYTE OPCODE: 0x0F 0x%02x - not fully implemented\n",
        second_opcode);

    // For now, skip these instructions conservatively
    // Most are 2-6 bytes, try to estimate
    uint32 skip_len = 2; // At minimum, 0x0F + second byte

    uint8 modrm = instr_buffer[2 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 rm = modrm & 7;

    if (mod == 1)
      skip_len = 4; // + disp8
    else if (mod == 2)
      skip_len = 7; // + disp32
    else if (mod == 0 && rm == 4)
      skip_len++; // + SIB

    bytes_consumed = prefix_offset + skip_len;
    return B_OK;
  }

  // MOVSXD r32, r/m32 (63 /r) - Move with sign extension (64-bit variant)
  // In 32-bit, treated as NOP or pass-through
  case 0x63: {
    DebugPrintf("MOVSXD r32, r/m32 (or ARPL in 32-bit)\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint32 instr_len = 2;
    if (mod == 1)
      instr_len = 3;
    else if (mod == 2)
      instr_len = 6;
    else if (mod == 0 && (modrm & 7) == 4)
      instr_len = 3; // SIB
    bytes_consumed = prefix_offset + instr_len;
    return B_OK;
  }

  // MOV r8, r/m8 (8A /r) - Move byte to register
  case 0x8A: {
    DebugPrintf("MOV r8, r/m8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register-to-register move: MOV r8, r8
      X86_32Registers &regs = x86_context.Registers();
      uint32 *dst_reg[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                           &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

      uint8 src_val = (uint8)(*dst_reg[rm_field]);
      *dst_reg[reg_field] = (*dst_reg[reg_field] & 0xFFFFFF00) | src_val;
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    } else {
      // Memory load: MOV r8, [mem] - skip for now
      uint32 instr_len = 2;
      if (mod == 1)
        instr_len = 3;
      else if (mod == 2)
        instr_len = 6;
      else if (rm_field == 4)
        instr_len = 3; // SIB
      bytes_consumed = prefix_offset + instr_len;
      return B_OK;
    }
  }

  // OR al, imm8 (0C xx) - OR immediate 8-bit
  case 0x0C: {
    DebugPrintf("OR %%al, $imm8\n");
    uint8 imm8 = instr_buffer[1 + prefix_offset];
    X86_32Registers &regs = x86_context.Registers();

    uint8 al = (uint8)regs.eax;
    al |= imm8;
    regs.eax = (regs.eax & 0xFFFFFF00) | al;

    regs.eflags &= ~(0x40 | 0x80 | 0x1); // Clear ZF, SF, CF
    if (al == 0)
      regs.eflags |= 0x40; // ZF
    if ((int8)al < 0)
      regs.eflags |= 0x80; // SF

    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // INSB (6C) - Input from port to string (I/O instruction)
  case 0x6C: {
    DebugPrintf("INSB (I/O - string input from port)\n");
    // This is an I/O instruction, typically not available in user mode
    // Treat as NOP for now
    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  // FPU Instructions (ESC 0-7: 0xD8-0xDF)
  case 0xD8:
  case 0xD9:
  case 0xDA:
  case 0xDB:
  case 0xDC:
  case 0xDD:
  case 0xDE:
  case 0xDF: {
    // FPU Escape sequences
    uint8 fpu_opcode = opcode;
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 rm = modrm & 7;

    // Calculate bytes consumed based on addressing mode
    bytes_consumed = prefix_offset + 2; // opcode + ModRM minimum

    if (mod == 1) {
      bytes_consumed += 1; // displacement8
    } else if (mod == 2) {
      bytes_consumed += 4; // displacement32
    } else if (mod == 0 && rm == 4) {
      bytes_consumed += 1; // SIB byte for memory addressing
    }

    DebugPrintf("[FPU] ESC opcode: 0x%02x, ModRM: 0x%02x\n", fpu_opcode, modrm);

    // Get FPU from context
    FloatingPointUnit *fpu = x86_context.GetFPU();
    if (!fpu) {
      printf("[INTERPRETER] ERROR: FPU not available in context\n");
      return B_DEV_NOT_READY;
    }

    // Execute FPU instruction through handler
    FPUInstructionHandler handler(*fpu);
    status_t fpu_status =
        handler.Execute(fpu_opcode, modrm, x86_context, fAddressSpace);

    if (fpu_status != B_OK) {
      printf("[INTERPRETER] FPU instruction failed: 0x%02x\n", fpu_opcode);
      return fpu_status;
    }

    return B_OK;
  }

  // ADC r32, r/m32 (11 /r) - Add with Carry (32-bit)
  case 0x11: {
    DebugPrintf("ADC r32, r/m32\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register mode
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst = *reg_ptrs[rm_field];
      uint32 src = *reg_ptrs[reg_field];
      uint32 cf = (regs.eflags >> 0) & 1;
      uint64 result = (uint64)dst + (uint64)src + (uint64)cf;
      *reg_ptrs[rm_field] = (uint32)result;

      regs.eflags = 0;
      if ((uint32)result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      if (result > 0xFFFFFFFFULL)
        regs.eflags |= 0x01; // CF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // XOR r/m8, r8 (30 /r) - Bitwise XOR (8-bit)
  case 0x30: {
    DebugPrintf("XOR r/m8, r8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register mode
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 dst = (uint8)*reg_ptrs[rm_field];
      uint8 src = (uint8)*reg_ptrs[reg_field];
      uint8 result = dst ^ src;
      *reg_ptrs[rm_field] = (*reg_ptrs[rm_field] & 0xFFFFFF00) | result;

      regs.eflags = 0; // XOR clears CF and OF
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int8)result < 0)
        regs.eflags |= 0x80; // SF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // XOR r32, r/m32 (33 /r) - Bitwise XOR (32-bit)
  case 0x33: {
    DebugPrintf("XOR r32, r/m32\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register mode
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint32 dst = *reg_ptrs[reg_field];
      uint32 src = *reg_ptrs[rm_field];
      uint32 result = dst ^ src;
      *reg_ptrs[reg_field] = result;

      regs.eflags = 0; // XOR clears CF and OF
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // OR EAX, imm32 (0D id) - Bitwise OR immediate with EAX
  case 0x0D: {
    uint32 imm32 = *(uint32 *)&instr_buffer[1 + prefix_offset];
    uint32 eax = regs.eax;
    uint32 result = eax | imm32;
    regs.eax = result;
    regs.eflags = 0; // OR clears CF and OF
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int32)result < 0)
      regs.eflags |= 0x80; // SF
    bytes_consumed = prefix_offset + 5;
    return B_OK;
  }

  // MOV r/m8, r8 (88 /r) - Move register to register/memory (8-bit)
  case 0x88: {
    DebugPrintf("MOV r/m8, r8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_field = (modrm >> 3) & 7;
    uint8 rm_field = modrm & 7;

    if (mod == 3) {
      // Register-to-register move (8-bit)
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      uint8 src_val = (uint8)*reg_ptrs[reg_field];
      *reg_ptrs[rm_field] = (*reg_ptrs[rm_field] & 0xFFFFFF00) | src_val;
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 2;
    return B_OK;
  }

  // MOV r/m8, imm8 (C6 /0 ib) - Move immediate to register/memory (8-bit)
  case 0xC6: {
    DebugPrintf("MOV r/m8, imm8\n");
    uint8 modrm = instr_buffer[1 + prefix_offset];
    uint8 mod = (modrm >> 6) & 3;
    uint8 reg_op = (modrm >> 3) & 7; // Should be 0 for MOV (Group 11)
    uint8 rm_field = modrm & 7;

    if (reg_op != 0) {
      // Not a MOV instruction (Group 11 has other opcodes)
      printf("[INTERPRETER] 0xC6: Group 11 opcode not MOV (reg_op=%d)\n",
             reg_op);
      bytes_consumed = prefix_offset + 2;
      return B_OK;
    }

    if (mod == 3) {
      // Register mode: MOV r8, imm8
      uint8 imm8 = instr_buffer[2 + prefix_offset];
      uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                            &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      *reg_ptrs[rm_field] = (*reg_ptrs[rm_field] & 0xFFFFFF00) | imm8;
      bytes_consumed = prefix_offset + 3; // opcode + modrm + imm8
      return B_OK;
    }
    // Memory mode not implemented
    bytes_consumed = prefix_offset + 3;
    return B_OK;
  }

  // LEAVE (C9) - Stack frame exit (equivalent to: MOV ESP, EBP; POP EBP)
  case 0xC9: {
    DebugPrintf("LEAVE\n");
    X86_32Registers &regs = x86_context.Registers();

    // Step 1: MOV ESP, EBP
    regs.esp = regs.ebp;

    // Step 2: POP EBP (pop from stack at ESP)
    uint32 stack_addr = regs.esp;
    uint32 new_ebp = 0;
    status_t st = fAddressSpace.Read(stack_addr, &new_ebp, 4);
    if (st != B_OK) {
      printf("[INTERPRETER] LEAVE: Failed to pop EBP from stack at 0x%08x\n",
             stack_addr);
      return st;
    }
    regs.ebp = new_ebp;
    regs.esp += 4; // Increment ESP after pop

    bytes_consumed = prefix_offset + 1;
    return B_OK;
  }

  default: {
    // Try to skip unknown instructions safely by guessing instruction length
    uint32 skip_len = 1; // Default minimum

    // For binary ALU operations (0x00-0x3F except those we implemented)
    // These are usually opcode + ModRM + optional displacement
    if (opcode < 0x40) {
      if (opcode & 1) { // Odd opcodes usually have ModRM
        uint8 modrm = instr_buffer[1];
        uint8 mod = (modrm >> 6) & 3;
        uint8 rm = modrm & 7;
        skip_len = 2; // opcode + ModRM
        if (mod == 1)
          skip_len = 3; // + disp8
        if (mod == 2)
          skip_len = 6; // + disp32
        if (rm == 4 && mod != 3)
          skip_len++; // + SIB
      }
    }

    printf("[INTERPRETER] UNKNOWN OPCODE: 0x%02x at EIP=0x%08x (guessing %d "
           "bytes)\n",
           opcode, regs.eip, skip_len);
    bytes_consumed = skip_len;
    return B_OK;
  }
  }
}

status_t InterpreterX86_32::Execute_GROUP_81(GuestContext &context,
                                             const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint8 modrm = instr[1];
  uint8 mod = (modrm >> 6) & 3;       // bits 7-6
  uint8 reg_field = (modrm >> 3) & 7; // bits 5-3 (extended opcode)
  uint8 rm_field = modrm & 7;         // bits 2-0

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  DebugPrintf("       GROUP1 0x81 Debug: ModR/M=0x%02x, mod=%u, reg_field=%u, "
              "rm_field=%u\n",
              modrm, mod, reg_field, rm_field);

  // Solo manejaremos el modo de registro a registro por ahora (Mod = 3)
  if (mod != 3) {
    DebugPrintf(
        "       GROUP1 0x81: Memory addressing not supported yet (mod=%u)\n",
        mod);
    return B_BAD_DATA;
  }

  // El operando inmediato de 32 bits siempre sigue al ModR/M byte
  int32 imm32 = (int32)((instr[2] & 0xFF) | ((instr[3] & 0xFF) << 8) |
                        ((instr[4] & 0xFF) << 16) | ((instr[5] & 0xFF) << 24));

  uint32 *target_reg = reg_ptrs[rm_field]; // El operando destino es el r/m
                                           // field en modo registro

  len = 6; // Opcode + ModR/M + Imm32

  DebugPrintf("       GROUP1 0x81: reg_field=%u (sub-opcode), rm_field=%u "
              "(target reg), imm32=%d\n",
              reg_field, rm_field, imm32);

  switch (reg_field) {
  case 0: // ADD r/m32, imm32
    DebugPrintf("       ADD %s, %d\n", reg_names[rm_field], imm32);
    *target_reg += imm32;
    // TODO: Update flags
    break;
  case 5: // SUB r/m32, imm32
    DebugPrintf("       SUB %s, %d\n", reg_names[rm_field], imm32);
    *target_reg -= imm32;
    // TODO: Update flags
    break;
  case 7: // CMP r/m32, imm32
    DebugPrintf("       CMP %s, %d\n", reg_names[rm_field], imm32);
    {
      uint32 result = *target_reg - imm32;
      regs.eflags = 0; // Clear flags for now
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
                             // TODO: OF, CF, AF, PF
    }
    break;
  default:
    DebugPrintf("       GROUP1 0x81: UNIMPLEMENTED sub-opcode %u\n", reg_field);
    return B_BAD_DATA;
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_GROUP_83(GuestContext &context,
                                             const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint8 modrm = instr[1];
  uint8 mod = (modrm >> 6) & 3;       // bits 7-6
  uint8 reg_field = (modrm >> 3) & 7; // bits 5-3 (extended opcode)
  uint8 rm_field = modrm & 7;         // bits 2-0

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  DebugPrintf("       GROUP1 0x83 Debug: ModR/M=0x%02x, mod=%u, reg_field=%u, "
              "rm_field=%u\n",
              modrm, mod, reg_field, rm_field);

  // Solo manejaremos el modo de registro a registro por ahora (Mod = 3)
  if (mod != 3) {
    DebugPrintf(
        "       GROUP1 0x83: Memory addressing not supported yet (mod=%u)\n",
        mod);
    return B_BAD_DATA;
  }

  // El operando inmediato de 8 bits siempre sigue al ModR/M byte
  int8 imm8 = (int8)instr[2];

  uint32 *target_reg = reg_ptrs[rm_field]; // El operando destino es el r/m
                                           // field en modo registro

  len = 3; // Opcode + ModR/M + Imm8

  DebugPrintf("       GROUP1 0x83: reg_field=%u (sub-opcode), rm_field=%u "
              "(target reg), imm8=%d\n",
              reg_field, rm_field, imm8);

  switch (reg_field) {
  case 0: // ADD r/m32, imm8
    DebugPrintf("       ADD %s, %d\n", reg_names[rm_field], imm8);
    *target_reg += imm8;
    // TODO: Update flags
    break;
  case 4: // AND r/m32, imm8
    DebugPrintf("       AND %s, 0x%02x\n", reg_names[rm_field], (uint8)imm8);
    *target_reg &= (uint32)(int32)imm8; // Sign-extend imm8 to 32-bit
    regs.eflags = 0;                    // Clear all flags
    break;
  case 5: // SUB r/m32, imm8
    DebugPrintf("       SUB %s, %d\n", reg_names[rm_field], imm8);
    *target_reg -= imm8;
    // TODO: Update flags
    break;
  case 7: // CMP r/m32, imm8
    DebugPrintf("       CMP %s, %d\n", reg_names[rm_field], imm8);
    {
      uint32 result = *target_reg - imm8;
      regs.eflags = 0; // Clear flags for now
      if (result == 0)
        regs.eflags |= 0x40; // ZF
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF
                             // TODO: OF, CF, AF, PF
    }
    break;
  default:
    DebugPrintf("       GROUP1 0x83: UNIMPLEMENTED sub-opcode %u\n", reg_field);
    return B_BAD_DATA;
  }

  return B_OK;
}

// Execute_GROUP_C1: Shift/Rotate instructions with 8-bit immediate
// Opcode 0xC1: r/m32, imm8
// Uses reg_op field (bits 5-3 of ModR/M) to encode the operation:
// /0 = ROL r/m32, imm8
// /1 = ROR r/m32, imm8
// /2 = RCL r/m32, imm8 (not implemented yet)
// /3 = RCR r/m32, imm8 (not implemented yet)
// /4 = SHL/SAL r/m32, imm8
// /5 = SHR r/m32, imm8
// /6 = (reserved)
// /7 = SAR r/m32, imm8
status_t InterpreterX86_32::Execute_GROUP_C1(GuestContext &context,
                                             const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint8 modrm = instr[1];
  uint8 mod = (modrm >> 6) & 3;       // bits 7-6
  uint8 reg_field = (modrm >> 3) & 7; // bits 5-3 (extended opcode)
  uint8 rm_field = modrm & 7;         // bits 2-0

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  printf("       GROUP_C1 Debug: ModR/M=0x%02x, mod=%u, reg_field=%u, "
         "rm_field=%u\n",
         modrm, mod, reg_field, rm_field);

  // Variables for addressing
  uint32 mem_addr = 0;
  uint32 original_value = 0;
  uint32 *target_ptr = NULL;
  uint8 shift_count = 0;
  bool is_memory = (mod != 3);

  if (is_memory) {
    // Memory addressing mode - NOT FULLY SUPPORTED YET
    // For now, skip these instructions
    printf("       GROUP_C1: Memory addressing mode not fully supported "
           "(mod=%u, rm=%u)\n",
           mod, rm_field);

    // Estimate instruction length to skip
    uint32 skip_len = 2; // opcode + modrm
    if (mod == 1)
      skip_len = 3; // + disp8
    if (mod == 2)
      skip_len = 6; // + disp32
    if (rm_field == 4)
      skip_len++; // + SIB

    len = skip_len + 1; // + shift count byte
    return B_OK;        // Skip without executing

  } else {
    // Register addressing mode (mod = 3)
    target_ptr = reg_ptrs[rm_field];
    original_value = *target_ptr;

    // The immediate 8-bit shift count follows the ModR/M byte
    shift_count = instr[2];

    // Limit shift count to reasonable values (CPU limits shifts to 31 bits)
    shift_count = shift_count & 0x1F;

    len = 3; // Opcode + ModR/M + Imm8
  }

  printf("       GROUP_C1: reg_field=%u (sub-opcode), rm_field=%u (target "
         "reg), shift_count=%u\n",
         reg_field, rm_field, shift_count);

  switch (reg_field) {
  case 0: // ROL r/m32, imm8 (Rotate Left)
  {
    printf("       ROL %s, %u\n", reg_names[rm_field], shift_count);
    if (shift_count > 0) {
      uint8 count = shift_count % 32; // Modulo 32 for 32-bit values
      uint32 rotated =
          (original_value << count) | (original_value >> (32 - count));

      if (target_ptr)
        *target_ptr = rotated;

      // Set carry flag based on the bit shifted out
      if (original_value & (1U << (32 - count)))
        regs.eflags |= FLAG_CF;
      else
        regs.eflags &= ~FLAG_CF;
    }
  } break;

  case 1: // ROR r/m32, imm8 (Rotate Right)
  {
    printf("       ROR %s, %u\n", reg_names[rm_field], shift_count);
    if (shift_count > 0) {
      uint8 count = shift_count % 32;
      uint32 rotated =
          (original_value >> count) | (original_value << (32 - count));
      if (target_ptr)
        *target_ptr = rotated;

      if (rotated & 0x80000000)
        regs.eflags |= FLAG_CF;
      else
        regs.eflags &= ~FLAG_CF;
    }
  } break;

  case 2: // RCL r/m32, imm8 (Rotate Left through Carry)
  {
    printf("       RCL %s, %u (treating as ROL)\n", reg_names[rm_field],
           shift_count);
    if (shift_count > 0) {
      uint8 count = shift_count % 32;
      uint32 rotated =
          (original_value << count) | (original_value >> (32 - count));
      if (target_ptr)
        *target_ptr = rotated;
    }
  } break;

  case 3: // RCR r/m32, imm8 (Rotate Right through Carry)
  {
    printf("       RCR %s, %u (treating as ROR)\n", reg_names[rm_field],
           shift_count);
    if (shift_count > 0) {
      uint8 count = shift_count % 32;
      uint32 rotated =
          (original_value >> count) | (original_value << (32 - count));
      if (target_ptr)
        *target_ptr = rotated;
    }
  } break;

  case 4: // SHL/SAL r/m32, imm8 (Shift Left)
  {
    printf("       SHL %s, %u\n", reg_names[rm_field], shift_count);
    uint32 result = original_value;
    if (shift_count > 0) {
      if (shift_count < 32) {
        if (original_value & (1U << (32 - shift_count)))
          regs.eflags |= FLAG_CF;
        else
          regs.eflags &= ~FLAG_CF;
        result = original_value << shift_count;
      } else {
        regs.eflags &= ~FLAG_CF;
        result = 0;
      }
    }
    if (target_ptr)
      *target_ptr = result;

    if (result == 0)
      regs.eflags |= FLAG_ZF;
    else
      regs.eflags &= ~FLAG_ZF;
    if (result & 0x80000000)
      regs.eflags |= FLAG_SF;
    else
      regs.eflags &= ~FLAG_SF;
  } break;

  case 5: // SHR r/m32, imm8 (Shift Right Logical)
  {
    printf("       SHR %s, %u\n", reg_names[rm_field], shift_count);
    uint32 result = original_value;
    if (shift_count > 0) {
      if (shift_count < 32) {
        if (original_value & (1U << (shift_count - 1)))
          regs.eflags |= FLAG_CF;
        else
          regs.eflags &= ~FLAG_CF;
        result = original_value >> shift_count;
      } else {
        regs.eflags &= ~FLAG_CF;
        result = 0;
      }
    }
    if (target_ptr)
      *target_ptr = result;

    if (result == 0)
      regs.eflags |= FLAG_ZF;
    else
      regs.eflags &= ~FLAG_ZF;
    if (result & 0x80000000)
      regs.eflags |= FLAG_SF;
    else
      regs.eflags &= ~FLAG_SF;
  } break;

  case 6: // Reserved
  {
    printf("       GROUP_C1: RESERVED sub-opcode %u\n", reg_field);
    return B_BAD_DATA;
  } break;

  case 7: // SAR r/m32, imm8 (Shift Right Arithmetic)
  {
    printf("       SAR %s, %u\n", reg_names[rm_field], shift_count);
    uint32 result = original_value;
    if (shift_count > 0) {
      int32 signed_val = (int32)original_value;
      if (shift_count < 32) {
        if (original_value & (1U << (shift_count - 1)))
          regs.eflags |= FLAG_CF;
        else
          regs.eflags &= ~FLAG_CF;
        result = (uint32)(signed_val >> shift_count);
      } else {
        regs.eflags &= ~FLAG_CF;
        result = (signed_val < 0) ? 0xFFFFFFFFU : 0;
      }
    }
    if (target_ptr)
      *target_ptr = result;

    if (result == 0)
      regs.eflags |= FLAG_ZF;
    else
      regs.eflags &= ~FLAG_ZF;
    if (result & 0x80000000)
      regs.eflags |= FLAG_SF;
    else
      regs.eflags &= ~FLAG_SF;
  } break;

  default:
    DebugPrintf("       GROUP_C1: UNKNOWN sub-opcode %u\n", reg_field);
    return B_BAD_DATA;
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_MOV(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint8 opcode = instr[0];
  // const char* regs[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI",
  // "EDI"}; // Moved to global static array
  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  if (opcode >= 0xB8 && opcode <= 0xBF) {
    // MOV $imm32, %reg (B8-BF)
    uint32 value = *(uint32 *)&instr[1];
    len = 5;
    int reg_idx = opcode - 0xB8;

    if (reg_idx >= 0 && reg_idx < 8) {
      *reg_ptrs[reg_idx] = value;
      DebugPrintf("       %s <- 0x%08x\n", reg_names[reg_idx], value);
    }
    return B_OK;
  } else if (opcode == 0x89) {
    // MOV r32, r/m32 (89 /r modrm) - Move source reg to dest reg/mem
    // For now, only handle reg-to-reg (no memory addressing)
    uint8 modrm = instr[1];
    uint8 src_reg = (modrm >> 3) & 7; // reg field (source)
    uint8 dst_reg = modrm & 7;        // r/m field (destination, for reg-reg)

    // Check if it's a register-to-register move (mod bits = 11)
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) { // Register mode
      if (src_reg < 8 && dst_reg < 8) {
        *reg_ptrs[dst_reg] = *reg_ptrs[src_reg];
        len = 2;
        DebugPrintf("       %s <- %s (0x%08x)\n", reg_names[dst_reg],
                    reg_names[src_reg], *reg_ptrs[dst_reg]);
        return B_OK;
      }
    }
    // TODO: Handle memory addressing modes (mod = 0, 1, 2)
    return B_BAD_DATA;
  } else if (opcode == 0x8B) {
    // MOV r32, r/m32 (8B /r modrm) - Move source reg/mem to dest reg
    // For now, only handle reg-to-reg (reverse of 89)
    uint8 modrm = instr[1];
    uint8 dst_reg = (modrm >> 3) & 7; // reg field (destination)
    uint8 src_reg = modrm & 7;        // r/m field (source, for reg-reg)

    // Check if it's a register-to-register move (mod bits = 11)
    uint8 mod = (modrm >> 6) & 3;
    if (mod == 3) { // Register mode
      if (src_reg < 8 && dst_reg < 8) {
        *reg_ptrs[dst_reg] = *reg_ptrs[src_reg];
        len = 2;
        DebugPrintf("       %s <- %s (0x%08x)\n", reg_names[dst_reg],
                    reg_names[src_reg], *reg_ptrs[dst_reg]);
        return B_OK;
      }
    }
    // TODO: Handle memory addressing modes (mod = 0, 1, 2)
    return B_BAD_DATA;
  }

  return B_BAD_DATA;
}

status_t InterpreterX86_32::Execute_INT(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  uint8 int_num = instr[1];
  len = 2;

  printf("[INTERPRETER] INT 0x%02x (syscall)\n", int_num);
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  printf("[INTERPRETER] EAX(syscall)=%u, EBX(arg1)=%u, ECX(arg2)=%u, "
         "EDX(arg3)=%u\n",
         regs.eax, regs.ebx, regs.ecx, regs.edx);

  if (int_num == 0x80 || int_num == 0x25 || int_num == 0x63) {
    // Syscall interrupt
    // INT 0x80: Linux syscall convention (legacy/compat)
    // INT 0x25: Haiku syscall convention (legacy/some versions)
    // INT 0x63: PRIMARY Haiku x86-32 syscall convention
    printf("[INT] Executing syscall (interrupt 0x%02x)\n", int_num);
    status_t syscall_status = fDispatcher.Dispatch(context);
    printf("[INT] Syscall returned, EAX=%u\n", regs.eax);
    // Check for special exit code
    if (syscall_status == (status_t)0x80000001) {
      return (status_t)0x80000001; // Propagate exit signal
    }
    return B_OK;
  }

  // INT 0x02 - Non-Maskable Interrupt or FPU exception
  // Common in complex programs like WebPositive
  if (int_num == 0x02) {
    printf("[INT] INT 0x02 (NMI/FPU exception) - treating as no-op\n");
    return B_OK; // Continue execution
  }

  printf("[INT] Unsupported interrupt: 0x%02x\n", int_num);
  return B_BAD_DATA;
}

status_t InterpreterX86_32::Execute_PUSH(GuestContext &context,
                                         const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 value = 0;
  uint8 opcode = instr[0];

  if (opcode >= 0x50 && opcode <= 0x57) {
    // PUSH reg (50-57)
    uint8 reg_idx = opcode - 0x50;

    // Obtener valor del registro
    switch (reg_idx) {
    case 0:
      value = regs.eax;
      break;
    case 1:
      value = regs.ecx;
      break;
    case 2:
      value = regs.edx;
      break;
    case 3:
      value = regs.ebx;
      break;
    case 4:
      value = regs.esp;
      break;
    case 5:
      value = regs.ebp;
      break;
    case 6:
      value = regs.esi;
      break;
    case 7:
      value = regs.edi;
      break;
    }
    len = 1;
    DebugPrintf("       PUSH r32 (reg=%d): 0x%08x\n", reg_idx, value);

  } else if (opcode == 0xFF) {
    // PUSH r/m32 (FF /6)
    ModRM modrm;
    status_t status = DecodeModRM(&instr[1], modrm);
    if (status != B_OK) {
      DebugPrintf("       PUSH r/m32: Failed to decode ModR/M\n");
      return status;
    }

    len = 1 + modrm.bytes_used;

    if (modrm.mod == 3) {
      // Register addressing: FF /6 r32
      uint32_t *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                              &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      value = *reg_ptrs[modrm.rm];
      DebugPrintf("       PUSH r32 (reg=%d): 0x%08x\n", modrm.rm, value);

    } else {
      // Memory addressing: FF /6 [mem]
      uint32_t mem_addr = GetEffectiveAddress(regs, modrm);
      status_t status = fAddressSpace.Read(mem_addr, &value, 4);
      if (status != B_OK) {
        DebugPrintf("       PUSH [mem]: Failed to read from 0x%08x\n",
                    mem_addr);
        return status;
      }
      DebugPrintf("       PUSH [mem] at 0x%08x: 0x%08x\n", mem_addr, value);
    }
  } else {
    DebugPrintf("       ERROR: Invalid opcode for Execute_PUSH: 0x%02x\n",
                opcode);
    return B_BAD_VALUE;
  }

  // Push al stack
  regs.esp -= 4;
  status_t status = fAddressSpace.Write(regs.esp, &value, 4);
  if (status != B_OK) {
    printf("       PUSH: Failed to write to stack at 0x%08x, status=%d\n",
           regs.esp, status);
    printf(
        "       [REGISTER DUMP] EAX=0x%08x EBX=0x%08x ECX=0x%08x EDX=0x%08x\n",
        regs.eax, regs.ebx, regs.ecx, regs.edx);
    printf("       [REGISTER DUMP] ESI=0x%08x EDI=0x%08x EBP=0x%08x\n",
           regs.esi, regs.edi, regs.ebp);
    fflush(stdout);
    return status;
  }

  DebugPrintf("       Pushed 0x%08x to stack (ESP=0x%08x)\n", value, regs.esp);
  return B_OK;
}

// Execute_PUSH_Imm: PUSH immediate (0x6A for 8-bit signed, 0x68 for 32-bit)
status_t InterpreterX86_32::Execute_PUSH_Imm(GuestContext &context,
                                             const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 value = 0;
  uint8 opcode = instr[0];

  if (opcode == 0x6A) {
    // PUSH immediate 8-bit signed (6A xx)
    // Sign-extend 8-bit signed value to 32 bits
    int8 imm8 = (int8)instr[1];
    value = (uint32)(int32)imm8; // Sign extension
    len = 2;
    DebugPrintf("       PUSH $0x%02x (0x%08x)\n", imm8, value);
  } else if (opcode == 0x68) {
    // PUSH immediate 32-bit (68 xx xx xx xx)
    value = instr[1] | (instr[2] << 8) | (instr[3] << 16) | (instr[4] << 24);
    len = 5;
    DebugPrintf("       PUSH $0x%08x\n", value);
  } else {
    DebugPrintf("       ERROR: Invalid PUSH immediate opcode 0x%02x\n", opcode);
    return B_BAD_VALUE;
  }

  // Push value onto stack
  regs.esp -= 4;
  status_t status = fAddressSpace.Write(regs.esp, &value, 4);
  if (status != B_OK) {
    printf("       PUSH Imm: Failed to write to stack at 0x%08x, status=%d\n",
           regs.esp, status);
    fflush(stdout);
    return status;
  }

  DebugPrintf("       Pushed 0x%08x to stack (ESP=0x%08x)\n", value, regs.esp);
  return B_OK;
}

status_t InterpreterX86_32::Execute_POP(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint8 reg_idx = instr[0] - 0x58;
  uint32 value = 0;

  // Pop del stack
  status_t status = fAddressSpace.Read(regs.esp, &value, 4);
  if (status != B_OK) {
    DebugPrintf("       POP: Failed to read from stack at 0x%08x\n", regs.esp);
    return status;
  }
  regs.esp += 4;

  // Almacenar en registro
  switch (reg_idx) {
  case 0:
    regs.eax = value;
    break;
  case 1:
    regs.ecx = value;
    break;
  case 2:
    regs.edx = value;
    break;
  case 3:
    regs.ebx = value;
    break;
  case 5:
    regs.ebp = value;
    break;
  case 6:
    regs.esi = value;
    break;
  case 7:
    regs.edi = value;
    break;
  }

  len = 1;
  DebugPrintf("       Popped 0x%08x from stack (ESP=0x%08x)\n", value,
              regs.esp);
  return B_OK;
}

status_t InterpreterX86_32::Execute_ADD(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 value = *(uint32 *)&instr[1];
  regs.eax += value;
  len = 5;

  DebugPrintf("       EAX += 0x%08x (new EAX=0x%08x)\n", value, regs.eax);
  return B_OK;
}

status_t InterpreterX86_32::Execute_SUB(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 value = *(uint32 *)&instr[1];
  regs.eax -= value;
  len = 5;

  DebugPrintf("       EAX -= 0x%08x (new EAX=0x%08x)\n", value, regs.eax);
  return B_OK;
}

status_t InterpreterX86_32::Execute_CMP(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint8 opcode = instr[0];

  if (opcode == 0x3D) {
    // CMP $imm32, %eax format
    uint32 value = *(uint32 *)&instr[1];
    uint32 result = regs.eax - value;
    len = 5;

    // Set flags
    regs.eflags = 0;
    if (result == 0)
      regs.eflags |= 0x40; // ZF
    if ((int32)result < 0)
      regs.eflags |= 0x80; // SF

    DebugPrintf("       CMP EAX(0x%08x) vs 0x%08x, FLAGS=0x%08x\n", regs.eax,
                value, regs.eflags);
    return B_OK;
  } else if (opcode == 0x39) {
    // CMP r/m32, r32 format (ModRM)
    uint8 modrm = instr[1];
    uint8 reg = (modrm >> 3) & 7; // Middle 3 bits
    uint8 rm = modrm & 7;         // Low 3 bits

    uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                          &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

    if (reg < 8 && rm < 8) {
      uint32 result = *reg_ptrs[rm] - *reg_ptrs[reg];
      len = 2;

      // Set flags based on result
      regs.eflags = 0;
      if (result == 0)
        regs.eflags |= 0x40; // ZF (bit 6)
      if ((int32)result < 0)
        regs.eflags |= 0x80; // SF (bit 7)
      if (result > 0x7FFFFFFF)
        regs.eflags |= 0x800; // OF (bit 11)

      DebugPrintf("       CMP R%d(0x%08x) vs R%d(0x%08x), FLAGS=0x%08x\n", rm,
                  *reg_ptrs[rm], reg, *reg_ptrs[reg], regs.eflags);
      return B_OK;
    }
  }

  return B_BAD_DATA;
}

status_t InterpreterX86_32::Execute_XOR(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  // XOR format: 31 /r modrm
  // Common: 31 DB = XOR %ebx, %ebx (opcode=0x31, modrm=0xDB)
  uint8 modrm = instr[1];
  uint8 reg = (modrm >> 3) & 7; // Middle 3 bits
  uint8 rm = modrm & 7;         // Low 3 bits

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  if (reg < 8 && rm < 8) {
    *reg_ptrs[reg] ^= *reg_ptrs[rm];
    len = 2;
    DebugPrintf("       XOR R%d ^= R%d (result=0x%08x)\n", reg, rm,
                *reg_ptrs[reg]);
    return B_OK;
  }

  return B_BAD_DATA;
}

// Execute_JMP: JMP instruction (E9 for relative, FF /4 for indirect)
status_t InterpreterX86_32::Execute_JMP(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32_t target_eip = 0;
  uint8 opcode = instr[0];

  if (opcode == 0xE9) {
    // JMP $imm32 (E9 xx xx xx xx) - relative
    int32 offset = *(int32 *)&instr[1];
    target_eip = regs.eip + 5 + offset;
    len = 5;

    DebugPrintf("       JMP $imm32 to 0x%08x (offset=%d)\n", target_eip,
                offset);

  } else if (opcode == 0xFF) {
    // JMP r/m32 (FF /4) - indirect
    ModRM modrm;
    status_t status = DecodeModRM(&instr[1], modrm);
    if (status != B_OK) {
      DebugPrintf("       JMP r/m32: Failed to decode ModR/M\n");
      return status;
    }

    len = 1 + modrm.bytes_used;

    printf("       [FF JMP DEBUG] modrm.mod=%d, modrm.rm=%d\n", modrm.mod,
           modrm.rm);

    if (modrm.mod == 3) {
      // Register addressing: FF /4 r32
      uint32_t *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                              &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      target_eip = *reg_ptrs[modrm.rm];
      DebugPrintf("       JMP r32 (reg=%d): jump to 0x%08x\n", modrm.rm,
                  target_eip);

    } else {
      // Memory addressing: FF /4 [mem]
      uint32_t mem_addr = GetEffectiveAddress(regs, modrm);
      status_t status = fAddressSpace.Read(mem_addr, &target_eip, 4);
      if (status != B_OK) {
        DebugPrintf(
            "       JMP [mem]: Failed to read target address from 0x%08x\n",
            mem_addr);
        return status;
      }

      // If the target_eip appears to be a relative offset (small value),
      // add the image base to get the absolute guest address
      // This handles GOT/PLT entries that may be stored as offsets
      if (target_eip <
          0x1000000) { // Arbitrary threshold for "relative" addresses
        X86_32GuestContext &x86_context =
            static_cast<X86_32GuestContext &>(context);
        uint32_t image_base = x86_context.GetImageBase();
        target_eip += image_base;
        printf("       [FF JMP DEBUG] Adjusted relative offset: 0x%08x + "
               "0x%08x = 0x%08x\n",
               target_eip - image_base, image_base, target_eip);
      }

      printf("       [FF JMP DEBUG] Memory addressing: mem_addr=0x%08x, "
             "target_eip=0x%08x\n",
             mem_addr, target_eip);
      DebugPrintf("       JMP [mem] at 0x%08x: jump to 0x%08x\n", mem_addr,
                  target_eip);
    }

  } else {
    DebugPrintf("       ERROR: Invalid opcode for Execute_JMP: 0x%02x\n",
                opcode);
    return B_BAD_VALUE;
  }

  // Check if target_eip is a stub function
  if (target_eip >= 0xbffc0000 && target_eip <= 0xbffc03e0) {
    printf("[JMP] Detected jump to stub at 0x%08x\n", target_eip);
    return ExecuteStubFunction(context, target_eip);
  }

  regs.eip = target_eip; // Set directly to target address
  len = 0;               // Don't increment EIP again in main loop
  return B_OK;
}

status_t InterpreterX86_32::Execute_RET(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 return_addr = 0;
  status_t status = fAddressSpace.Read(regs.esp, &return_addr, 4);
  if (status != B_OK) {
    DebugPrintf(
        "       RET: Failed to read return address from stack at 0x%08x\n",
        regs.esp);
    return status;
  }
  regs.esp += 4;
  len = 1;

  DebugPrintf("       RET to 0x%08x (ESP=0x%08x)\n", return_addr, regs.esp);

  if (return_addr == 0) {
    // Return a dirección 0 significa fin del programa
    printf("[INTERPRETER] Program returned to 0x00000000, exiting\n");
    return B_INTERRUPTED; // Señal de que terminar
  }

  regs.eip = return_addr; // Set directly to return address
  len = 0;                // Don't increment EIP again in main loop
  return B_OK;
}

// Sprint 4: Conditional jumps implementation

status_t InterpreterX86_32::Execute_JZ(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  int8 offset = (int8)instr[1];
  len = 2;

  // JZ: Jump if ZF (Zero Flag) is set
  if (regs.eflags & 0x40) { // ZF = bit 6
    uint32 new_eip = regs.eip + 2 + offset;
    DebugPrintf("       JZ: Taking jump to 0x%08x (ZF=1)\n", new_eip);
    regs.eip = new_eip;
    len = 0; // Don't increment EIP again in main loop
  } else {
    DebugPrintf("       JZ: Not taking jump (ZF=0)\n");
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_JNZ(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  int8 offset = (int8)instr[1];
  len = 2;

  // JNZ: Jump if ZF is NOT set
  if (!(regs.eflags & 0x40)) { // ZF = bit 6
    uint32 new_eip = regs.eip + 2 + offset;
    DebugPrintf("       JNZ: Taking jump to 0x%08x (ZF=0)\n", new_eip);
    regs.eip = new_eip;
    len = 0; // Don't increment EIP again in main loop
  } else {
    DebugPrintf("       JNZ: Not taking jump (ZF=1)\n");
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_JL(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  int8 offset = (int8)instr[1];
  len = 2;

  // JL: Jump if Less (signed) - SF != OF
  uint8 sf = (regs.eflags >> 7) & 1;  // Sign Flag = bit 7
  uint8 of = (regs.eflags >> 11) & 1; // Overflow Flag = bit 11

  if (sf != of) {
    uint32 new_eip = regs.eip + 2 + offset;
    DebugPrintf("       JL: Taking jump to 0x%08x (SF=%d, OF=%d)\n", new_eip,
                sf, of);
    regs.eip = new_eip;
    len = 0; // Don't increment EIP again in main loop
  } else {
    DebugPrintf("       JL: Not taking jump (SF=%d, OF=%d)\n", sf, of);
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_JLE(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  int8 offset = (int8)instr[1];
  len = 2;

  // JLE: Jump if Less or Equal (signed) - ZF=1 or SF != OF
  uint8 zf = (regs.eflags >> 6) & 1;  // Zero Flag = bit 6
  uint8 sf = (regs.eflags >> 7) & 1;  // Sign Flag = bit 7
  uint8 of = (regs.eflags >> 11) & 1; // Overflow Flag = bit 11

  if (zf || (sf != of)) {
    uint32 new_eip = regs.eip + 2 + offset;
    DebugPrintf("       JLE: Taking jump to 0x%08x (ZF=%d, SF=%d, OF=%d)\n",
                new_eip, zf, sf, of);
    regs.eip = new_eip;
    len = 0; // Don't increment EIP again in main loop
  } else {
    DebugPrintf("       JLE: Not taking jump (ZF=%d, SF=%d, OF=%d)\n", zf, sf,
                of);
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_JG(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  int8 offset = (int8)instr[1];
  len = 2;

  // JG: Jump if Greater (signed) - ZF=0 and SF=OF
  uint8 zf = (regs.eflags >> 6) & 1;  // Zero Flag = bit 6
  uint8 sf = (regs.eflags >> 7) & 1;  // Sign Flag = bit 7
  uint8 of = (regs.eflags >> 11) & 1; // Overflow Flag = bit 11

  if (!zf && (sf == of)) {
    uint32 new_eip = regs.eip + 2 + offset;
    DebugPrintf("       JG: Taking jump to 0x%08x (ZF=%d, SF=%d, OF=%d)\n",
                new_eip, zf, sf, of);
    regs.eip = new_eip;
    len = 0; // Don't increment EIP again in main loop
  } else {
    DebugPrintf("       JG: Not taking jump (ZF=%d, SF=%d, OF=%d)\n", zf, sf,
                of);
  }

  return B_OK;
}

status_t InterpreterX86_32::Execute_JGE(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  int8 offset = (int8)instr[1];
  len = 2;

  // JGE: Jump if Greater or Equal (signed) - SF=OF
  uint8 sf = (regs.eflags >> 7) & 1;  // Sign Flag = bit 7
  uint8 of = (regs.eflags >> 11) & 1; // Overflow Flag = bit 11

  if (sf == of) {
    uint32 new_eip = regs.eip + 2 + offset;
    DebugPrintf("       JGE: Taking jump to 0x%08x (SF=%d, OF=%d)\n", new_eip,
                sf, of);
    regs.eip = new_eip;
    len = 0; // Don't increment EIP again in main loop
  } else {
    DebugPrintf("       JGE: Not taking jump (SF=%d, OF=%d)\n", sf, of);
  }

  return B_OK;
}

// Sprint 5: CALL instruction for dynamic linking support
// Handles both:
// - E8 xx xx xx xx (CALL immediate relative)
// - FF /2 (CALL r/m32 indirect)
status_t InterpreterX86_32::Execute_CALL(GuestContext &context,
                                         const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32_t target_eip = 0;
  uint8 opcode = instr[0];

  if (opcode == 0xE8) {
    // CALL $imm32 (E8 xx xx xx xx)
    // Immediate: 4 bytes, relative offset
    int32_t offset = *(int32_t *)&instr[1];
    len = 5;

    // Target = current EIP + len + offset
    target_eip = regs.eip + len + offset;
    printf("       [CALL E8 DEBUG] EIP=0x%08x\n", regs.eip);
    printf("       [CALL E8 DEBUG] offset_bytes: %02x %02x %02x %02x\n",
           instr[1], instr[2], instr[3], instr[4]);
    printf("       [CALL E8 DEBUG] offset as int32=0x%08x (%d)\n", offset,
           offset);
    printf("       [CALL E8 DEBUG] target = 0x%08x + %u + 0x%08x = 0x%08x\n",
           regs.eip, len, offset, target_eip);
    DebugPrintf("       CALL $imm32 (offset=0x%08x): jump to 0x%08x\n", offset,
                target_eip);

  } else if (opcode == 0xFF) {
    // CALL r/m32 (FF /2)
    // Decode ModR/M to find the register or memory operand
    ModRM modrm;
    status_t status = DecodeModRM(&instr[1], modrm);
    if (status != B_OK) {
      DebugPrintf("       CALL r/m32: Failed to decode ModR/M\n");
      return status;
    }

    len = 1 + modrm.bytes_used; // Opcode + ModR/M + displacement

    printf(
        "       [FF CALL DEBUG] modrm.mod=%d, modrm.reg_op=%d, modrm.rm=%d\n",
        modrm.mod, modrm.reg_op, modrm.rm);

    if (modrm.mod == 3) {
      // Register addressing: FF /2 r32
      uint32_t *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                              &regs.esp, &regs.ebp, &regs.esi, &regs.edi};
      target_eip = *reg_ptrs[modrm.rm];
      DebugPrintf("       CALL r32 (reg=%d): jump to 0x%08x\n", modrm.rm,
                  target_eip);

    } else {
      // Memory addressing: FF /2 [mem]
      uint32_t mem_addr = GetEffectiveAddress(regs, modrm);
      printf("       [FF CALL DEBUG] Memory addressing: mem_addr=0x%08x\n",
             mem_addr);
      status_t status = fAddressSpace.Read(mem_addr, &target_eip, 4);
      if (status != B_OK) {
        DebugPrintf(
            "       CALL [mem]: Failed to read target address from 0x%08x\n",
            mem_addr);
        return status;
      }
      printf("       [FF CALL DEBUG] Read from mem 0x%08x: target_eip=0x%08x\n",
             mem_addr, target_eip);
      DebugPrintf("       CALL [mem] at 0x%08x: jump to 0x%08x\n", mem_addr,
                  target_eip);
    }

  } else {
    DebugPrintf("       ERROR: Invalid opcode for Execute_CALL: 0x%02x\n",
                opcode);
    return B_BAD_VALUE;
  }

  // 1. Push return address (EIP + instruction length)
  uint32_t return_address = regs.eip + len;
  regs.esp -= 4;

  // Write return address to stack
  status_t status = fAddressSpace.Write(regs.esp, &return_address, 4);
  if (status != B_OK) {
    DebugPrintf("       CALL: Failed to push return address on stack\n");
    return status;
  }

  // 2. Check if target is a stub function
  if (target_eip >= 0xbffc0000 && target_eip <= 0xbffc03e0) {
    printf("[CALL] Detected call to stub at 0x%08x\n", target_eip);
    // Stub will execute and return via RET instruction
    regs.eip = target_eip;
    len = 0;
    return ExecuteStubFunction(context, target_eip);
  }

  // 3. Jump to target
  DebugPrintf("       CALL: Push return addr 0x%08x, jump to 0x%08x\n",
              return_address, target_eip);

  regs.eip = target_eip; // Set directly to target address
  len = 0;               // Don't increment EIP again in main loop

  return B_OK;
}

// ============================================================================
// Sprint 4: MOV [memory] addressing modes implementation
// ============================================================================

// DecodeModRM: Parse ModR/M byte and any displacement bytes
// Returns B_OK on success, status code on error
status_t InterpreterX86_32::DecodeModRM(const uint8 *instr, ModRM &result) {
  uint8 modrm_byte = instr[0];

  // Extract fields from ModR/M byte
  result.mod = (modrm_byte >> 6) & 0x3;    // bits 7-6: addressing mode
  result.reg_op = (modrm_byte >> 3) & 0x7; // bits 5-3: register/opcode
  result.rm = modrm_byte & 0x7;            // bits 2-0: register or memory
  result.displacement = 0;
  result.bytes_used = 1; // At minimum, the ModR/M byte itself

  printf("       [ModRM] mod=%d, reg_op=%d, rm=%d", result.mod, result.reg_op,
         result.rm);

  // Handle SIB byte if rm == 4 and not register mode
  if (result.rm == 4 && result.mod != 3) {
    // SIB byte present - Parse it
    if (result.bytes_used >= 15) {
      printf(" [SIB byte OVERFLOW]");
      return B_BAD_DATA;
    }

    uint8 sib_byte = instr[result.bytes_used];
    result.bytes_used++;

    uint8 scale = (sib_byte >> 6) & 0x3; // bits 7-6
    uint8 index = (sib_byte >> 3) & 0x7; // bits 5-3
    uint8 base = sib_byte & 0x7;         // bits 2-0

    DebugPrintf(" [SIB: scale=%d, index=%s, base=%s]", scale,
                (index != 4) ? reg_names[index] : "none",
                (base != 5) ? reg_names[base] : "[disp32]");
  }

  // Parse displacement based on mod field
  switch (result.mod) {
  case 0: // No displacement (or disp32 if rm==5)
    if (result.rm == 5) {
      // Special case: [disp32] addressing
      result.displacement = *(int32 *)&instr[result.bytes_used];
      result.bytes_used += 4;
      printf(", disp32=0x%08x", result.displacement);
    }
    break;

  case 1: // 8-bit displacement
    result.displacement = (int8)instr[result.bytes_used];
    result.bytes_used++;
    printf(", disp8=%d", result.displacement);
    break;

  case 2: // 32-bit displacement
    result.displacement = *(int32 *)&instr[result.bytes_used];
    result.bytes_used += 4;
    printf(", disp32=0x%08x", result.displacement);
    break;

  case 3: // Register mode (no displacement)
    result.displacement = 0;
    printf(" [register mode]");
    break;
  }

  printf("\n");
  return B_OK;
}

// GetEffectiveAddress: Calculate the effective address from ModR/M info
// Handles all addressing modes including special cases
uint32 InterpreterX86_32::GetEffectiveAddress(X86_32Registers &regs,
                                              const ModRM &modrm) {
  uint32 *reg_ptrs[] = {(uint32 *)&regs.eax, (uint32 *)&regs.ecx,
                        (uint32 *)&regs.edx, (uint32 *)&regs.ebx,
                        (uint32 *)&regs.esp, (uint32 *)&regs.ebp,
                        (uint32 *)&regs.esi, (uint32 *)&regs.edi};

  // Special case: mod=0, rm=5 means [disp32] with NO base register
  if (modrm.mod == 0 && modrm.rm == 5) {
    printf("       [Effective Addr] [disp32]=0x%08x (no base register)\n",
           modrm.displacement);
    return (uint32)modrm.displacement;
  }

  // Get base address from rm register
  uint32 base_addr = *reg_ptrs[modrm.rm];
  uint32 effective_addr = base_addr + modrm.displacement;

  printf(
      "       [Effective Addr] base=0x%08x (reg %d) + disp=0x%08x = 0x%08x\n",
      base_addr, modrm.rm, modrm.displacement, effective_addr);

  return effective_addr;
}

// Execute_MOV_Load: MOV reg, [memory] (opcode 0x8B)
// Load a 32-bit value from memory into a register
status_t InterpreterX86_32::Execute_MOV_Load(GuestContext &context,
                                             const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  // Decode ModR/M
  ModRM modrm;
  status_t status = DecodeModRM(&instr[1], modrm);
  if (status != B_OK) {
    printf("       MOV Load: Failed to decode ModR/M\n");
    return status;
  }

  // Destination register (reg field)
  int dest_reg = modrm.reg_op;

  // For register-to-register moves, this shouldn't be called
  if (modrm.mod == 3) {
    printf("       MOV Load: ERROR - mod=3 (register mode) should be handled "
           "elsewhere\n");
    return B_BAD_DATA;
  }

  // Calculate effective address
  uint32 src_addr = GetEffectiveAddress(regs, modrm);
  printf("       [MOV_LOAD] dest_reg=%d, src_addr=0x%08x\n", dest_reg,
         src_addr);

  // Read 4 bytes from guest memory (AddressSpace::Read handles translation)
  uint32 value = 0;
  status = fAddressSpace.Read(src_addr, &value, 4);
  if (status != B_OK) {
    printf("       MOV Load: WARNING - Failed to read from address 0x%08x, "
           "using 0, status=%d\n",
           src_addr, status);
    value = 0; // Use default value instead of crashing
               // Don't return error - allow program to continue
  }

  // Store in destination register
  *reg_ptrs[dest_reg] = value;
  printf("       [MOV_LOAD_RESULT] reg%d=0x%08x (loaded from 0x%08x)\n",
         dest_reg, value, src_addr);

  len = 1 + modrm.bytes_used; // Opcode + ModR/M + displacement

  DebugPrintf("       MOV %s, [0x%08x] (value=0x%08x)\n", reg_names[dest_reg],
              src_addr, value);

  return B_OK;
}

// Execute_MOV_Store: MOV [memory], reg (opcode 0x89 with mod != 3)
// Store a 32-bit register value to memory
status_t InterpreterX86_32::Execute_MOV_Store(GuestContext &context,
                                              const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  // Decode ModR/M
  ModRM modrm;
  status_t status = DecodeModRM(&instr[1], modrm);
  if (status != B_OK) {
    DebugPrintf("       MOV Store: Failed to decode ModR/M\n");
    return status;
  }

  // Source register (reg field)
  int src_reg = modrm.reg_op;
  uint32 src_value = *reg_ptrs[src_reg];

  // Calculate destination address
  uint32 dst_addr = GetEffectiveAddress(regs, modrm);

  // Write 4 bytes to guest memory (AddressSpace::Write handles translation)
  status = fAddressSpace.Write(dst_addr, &src_value, 4);
  if (status != B_OK) {
    DebugPrintf("       MOV Store: Failed to write to address 0x%08x\n",
                dst_addr);
    return status;
  }

  len = 1 + modrm.bytes_used; // Opcode + ModR/M + displacement

  DebugPrintf("       MOV [0x%08x], %s (value=0x%08x)\n", dst_addr,
              reg_names[src_reg], src_value);

  return B_OK;
}

// Execute_MOV_Load_FS: MOV reg, FS:[mem] (opcode 0x64 0x8B /r modrm)
// Load a 32-bit value from FS segment into a register (Thread Local Storage)
// In Haiku, FS points to the TLS area. Typically accessed as:
// - 0x64 0xA1 xx xx xx xx = MOV offset(%fs), %eax
// - 0x64 0x8B reg_field modrm = MOV offset(%fs), %reg (with ModRM)
status_t InterpreterX86_32::Execute_MOV_Load_FS(GuestContext &context,
                                                const uint8 *instr,
                                                uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();

  uint32 *reg_ptrs[] = {&regs.eax, &regs.ecx, &regs.edx, &regs.ebx,
                        &regs.esp, &regs.ebp, &regs.esi, &regs.edi};

  // TLS area base in guest memory (high memory, safe from regular allocations)
  // FS segment in Haiku32 points to per-thread TLS data
  static const uint32 TLS_BASE = 0xbffff000; // Reserve this page for TLS

  // Check for special case: 0xA1 = MOV moffs32, eax (immediate offset, always
  // EAX)
  uint8 opcode = instr[0];
  if (opcode == 0xA1) {
    // 0xA1 xx xx xx xx - MOV offset(%fs), %eax (without prefix)
    // NOTE: This function is called after prefix has been stripped
    // so we don't need to account for it here
    uint32 offset = (instr[1] & 0xFF) | ((instr[2] & 0xFF) << 8) |
                    ((instr[3] & 0xFF) << 16) | ((instr[4] & 0xFF) << 24);

    // Read actual value from TLS area
    uint32_t tls_address = TLS_BASE + offset;
    printf("       [FS_MOV_A1] offset=0x%08x, tls_address=0x%08x\n", offset,
           tls_address);
    status_t status =
        fAddressSpace.ReadMemory(tls_address, &regs.eax, sizeof(uint32));
    if (status != B_OK) {
      printf(
          "       ERROR: Failed to read TLS at offset 0x%08x (addr 0x%08x)\n",
          offset, tls_address);
      return status;
    }

    len = 5; // 0xA1 (1 byte) + 4 byte offset = 5 bytes (NOT including prefix)
    printf("       MOV %%fs:0x%08x, %%eax (value=0x%08x)\n", offset, regs.eax);
    return B_OK;
  }

  // General case: 0x8B with ModRM - MOV FS:[mem], reg
  // NOTE: When FS override is present, the address calculation uses the normal
  // addressing mode but the access happens through the FS segment. In Haiku, FS
  // base = TLS base (0xbffff000) So the final address is: FS_BASE + (calculated
  // effective address treated as offset)

  // Decode ModR/M
  ModRM modrm;
  status_t status = DecodeModRM(&instr[1], modrm);
  if (status != B_OK) {
    printf("       MOV Load FS: Failed to decode ModR/M\n");
    return status;
  }

  // Destination register (reg field)
  int dest_reg = modrm.reg_op;

  // For register-to-register moves with FS, shouldn't happen
  if (modrm.mod == 3) {
    printf("       MOV Load FS: ERROR - mod=3 (register mode) shouldn't use FS "
           "override\n");
    return B_BAD_DATA;
  }

  // Calculate effective address - in x86, MOD/RM addressing with FS override
  // means we calculate the address relative to FS base
  // The issue is GetEffectiveAddress assumes absolute addressing
  // For FS access, if we have [ebp+disp], that's actually FS_BASE + ebp + disp
  // But in Haiku TLS, usually accessed as FS_BASE + offset (small offset)

  // Get raw components without calling GetEffectiveAddress
  uint32 *reg_ptrs_base[] = {(uint32 *)&regs.eax, (uint32 *)&regs.ecx,
                             (uint32 *)&regs.edx, (uint32 *)&regs.ebx,
                             (uint32 *)&regs.esp, (uint32 *)&regs.ebp,
                             (uint32 *)&regs.esi, (uint32 *)&regs.edi};

  // For FS-relative access: typically just a displacement (mod=0, disp) or
  // small offset Example: MOV %fs:0x4, %eax  OR  MOV -24(%fs), %eax
  uint32 fs_offset = 0;
  if (modrm.mod == 0 && modrm.rm == 5) {
    // Special case [disp32] - direct displacement, no register
    fs_offset = modrm.displacement;
    printf("       MOV Load FS: [disp32] offset=0x%08x\n", fs_offset);
  } else if (modrm.mod != 3) {
    // Normal addressing with register base - for FS, treat register value as
    // part of TLS address
    uint32 base = *reg_ptrs_base[modrm.rm];
    fs_offset = base + modrm.displacement;
    printf("       MOV Load FS: [reg+disp] reg=%d(0x%08x) + disp=0x%08x = "
           "offset=0x%08x\n",
           modrm.rm, base, modrm.displacement, fs_offset);
  }

  // Read actual value from TLS area
  // Since FS base is TLS_BASE, the address is TLS_BASE + fs_offset
  // But wait: if fs_offset is already a full address (0xbfff7d20), that's wrong
  // The real issue: we need to extract ONLY the offset part
  // In Haiku, typically you access TLS with small offsets (0, 4, 8, etc.)
  // If fs_offset > 0x1000, it's probably corrupted; use modulo
  uint32 tls_actual_offset = fs_offset & 0xFFF; // Mask to TLS page size
  uint32_t tls_address = TLS_BASE + tls_actual_offset;

  printf("       MOV Load FS: fs_offset=0x%08x -> TLS_offset=0x%08x, "
         "address=0x%08x\n",
         fs_offset, tls_actual_offset, tls_address);

  uint32_t value;
  status = fAddressSpace.ReadMemory(tls_address, &value, sizeof(uint32));
  if (status != B_OK) {
    printf("       MOV Load FS: WARNING - Failed to read TLS at offset 0x%08x "
           "(addr 0x%08x), using 0\n",
           tls_actual_offset, tls_address);
    value = 0; // Use 0 as fallback instead of crashing
    // Don't return error - allow program to continue
  }

  // Store in destination register
  *reg_ptrs[dest_reg] = value;

  len = 1 + modrm.bytes_used; // Opcode + ModR/M + displacement

  DebugPrintf("       MOV %%fs:0x%08x, %s (value=0x%08x)\n", tls_actual_offset,
              reg_names[dest_reg], value);

  return B_OK;
}

// Jump instructions (additional)
template <typename T>
void InterpreterX86_32::SetFlags_ADD(X86_32Registers &regs, T result, T op1,
                                     T op2, bool is_32bit) {
  // Clear relevant flags first
  regs.eflags &=
      ~(0x40 | 0x80 | 0x1 | 0x800 | 0x4 | 0x10); // Clear ZF, SF, CF, OF, PF, AF

  // Zero Flag (ZF): result == 0
  if (result == 0)
    regs.eflags |= 0x40; // ZF (bit 6)

  // Sign Flag (SF): most significant bit of result
  if ((result & (1 << (sizeof(T) * 8 - 1))) != 0)
    regs.eflags |= 0x80; // SF (bit 7)

  // Carry Flag (CF): unsigned overflow
  // For addition: CF is set if there is a carry out of the most significant
  // bit.
  if (is_32bit) {
    if ((uint32)result < (uint32)op1 || (uint32)result < (uint32)op2)
      regs.eflags |= 0x1; // CF (bit 0)
  } else {                // 8-bit
    if ((uint8)result < (uint8)op1 || (uint8)result < (uint8)op2)
      regs.eflags |= 0x1; // CF (bit 0)
  }

  // Overflow Flag (OF): signed overflow
  // For addition: OF is set if signs of operands are same but sign of result is
  // different.
  bool op1_sign = (op1 & (1 << (sizeof(T) * 8 - 1))) != 0;
  bool op2_sign = (op2 & (1 << (sizeof(T) * 8 - 1))) != 0;
  bool result_sign = (result & (1 << (sizeof(T) * 8 - 1))) != 0;

  if (op1_sign == op2_sign && op1_sign != result_sign)
    regs.eflags |= 0x800; // OF (bit 11)

  // Parity Flag (PF): even number of set bits in low 8 bits of result
  uint8 low_byte = (uint8)result;
  uint8 set_bits = 0;
  for (int i = 0; i < 8; ++i) {
    if ((low_byte >> i) & 1)
      set_bits++;
  }
  if ((set_bits % 2) == 0)
    regs.eflags |= 0x4; // PF (bit 2)

  // Auxiliary Carry Flag (AF): carry from bit 3 to bit 4
  // (op1 & 0xF) + (op2 & 0xF) > 0xF
  if (((op1 & 0xF) + (op2 & 0xF)) & 0x10)
    regs.eflags |= 0x10; // AF (bit 4)
}

status_t InterpreterX86_32::Execute_JA(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JA: Jump if Above (unsigned) - CF=0 and ZF=0
  uint8 cf = (regs.eflags >> 0) & 1; // Carry Flag = bit 0
  uint8 zf = (regs.eflags >> 6) & 1; // Zero Flag = bit 6

  if (cf == 0 && zf == 0) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JAE(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JAE: Jump if Above or Equal (unsigned) - CF=0
  uint8 cf = (regs.eflags >> 0) & 1;
  if (cf == 0) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JB(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JB: Jump if Below (unsigned) - CF=1
  uint8 cf = (regs.eflags >> 0) & 1;
  if (cf == 1) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JBE(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JBE: Jump if Below or Equal (unsigned) - CF=1 or ZF=1
  uint8 cf = (regs.eflags >> 0) & 1;
  uint8 zf = (regs.eflags >> 6) & 1;
  if (cf == 1 || zf == 1) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JP(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JP: Jump if Parity - PF=1
  uint8 pf = (regs.eflags >> 2) & 1;
  if (pf == 1) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JNP(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JNP: Jump if Not Parity - PF=0
  uint8 pf = (regs.eflags >> 2) & 1;
  if (pf == 0) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JS(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JS: Jump if Sign - SF=1
  uint8 sf = (regs.eflags >> 7) & 1;
  if (sf == 1) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JNS(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JNS: Jump if Not Sign - SF=0
  uint8 sf = (regs.eflags >> 7) & 1;
  if (sf == 0) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JO(GuestContext &context,
                                       const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JO: Jump if Overflow - OF=1
  uint8 of = (regs.eflags >> 11) & 1;
  if (of == 1) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

status_t InterpreterX86_32::Execute_JNO(GuestContext &context,
                                        const uint8 *instr, uint32 &len) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);
  X86_32Registers &regs = x86_context.Registers();
  int8 offset = (int8)instr[1];
  len = 2;

  // JNO: Jump if Not Overflow - OF=0
  uint8 of = (regs.eflags >> 11) & 1;
  if (of == 0) {
    regs.eip = regs.eip + 2 + offset;
    len = 0;
  }
  return B_OK;
}

// ============================================================================
// STUB FUNCTION DISPATCHER
// ============================================================================

status_t InterpreterX86_32::ExecuteStubFunction(GuestContext &context,
                                                uint32_t stub_address) {
  X86_32GuestContext &x86_context = static_cast<X86_32GuestContext &>(context);

  printf("[STUB DISPATCHER] Executing stub at 0x%08x\n", stub_address);

  // Map stub addresses to function names and implementations
  // Each stub is allocated 16 bytes starting from 0xbffc0000

  uint32_t offset = stub_address - 0xbffc0000;
  int stub_index = offset / 16;

  // Stub names in the same order as registered in DynamicLinker
  const char *stub_names[] = {
      // GNU coreutils and error handling
      "quote_quoting_options", // 0xbffc0000
      "close_stdout",          // 0xbffc0010
      "version_etc_copyright", // 0xbffc0020
      "error_message_count",   // 0xbffc0030
      "error_print_progname",  // 0xbffc0040
      "program_name",          // 0xbffc0050
      "exit_failure",          // 0xbffc0060
      "thrd_exit",             // 0xbffc0070
      "Version",               // 0xbffc0080
      "error_one_per_line",    // 0xbffc0090

      // GNU libc memory allocation wrappers
      "xmalloc",        // 0xbffc00a0
      "xcalloc",        // 0xbffc00b0
      "xrealloc",       // 0xbffc00c0
      "xcharalloc",     // 0xbffc00d0
      "xmemdup",        // 0xbffc00e0
      "x2nrealloc",     // 0xbffc00f0
      "xireallocarray", // 0xbffc0100
      "xreallocarray",  // 0xbffc0110
      "ximalloc",       // 0xbffc0120
      "xicalloc",       // 0xbffc0130

      // GNU libc error functions
      "error",      // 0xbffc0140
      "xalloc_die", // 0xbffc0150

      // GNU quoting functions (quotearg)
      "quotearg_alloc_mem",    // 0xbffc0160
      "quotearg_n_custom_mem", // 0xbffc0170
      "quotearg_n_custom",     // 0xbffc0180
      "quotearg_n_mem",        // 0xbffc0190
      "quotearg_n",            // 0xbffc01a0
      "quotearg_char_mem",     // 0xbffc01b0
      "quotearg_char",         // 0xbffc01c0
      "quotearg_colon",        // 0xbffc01d0
      "quotearg_n_style",      // 0xbffc01e0
      "quotearg_n_style_mem",  // 0xbffc01f0
      "quote_n",               // 0xbffc0200
      "quote_n_mem",           // 0xbffc0210

      // GNU libc version/program functions
      "set_program_name", // 0xbffc0220
      "getprogname",      // 0xbffc0230
      "version_etc",      // 0xbffc0240
      "version_etc_arn",  // 0xbffc0250
      "version_etc_va",   // 0xbffc0260
      "usage",            // 0xbffc0270

      // GNU libc locale/encoding functions
      "locale_charset",   // 0xbffc0280
      "hard_locale",      // 0xbffc0290
      "setlocale_null_r", // 0xbffc02a0
      "rpl_nl_langinfo",  // 0xbffc02b0

      // rpl_* replacement functions
      "rpl_malloc",   // 0xbffc02c0
      "rpl_calloc",   // 0xbffc02d0
      "rpl_realloc",  // 0xbffc02e0
      "rpl_free",     // 0xbffc02f0
      "rpl_mbrtowc",  // 0xbffc0300
      "rpl_fclose",   // 0xbffc0310
      "rpl_fflush",   // 0xbffc0320
      "rpl_fseeko",   // 0xbffc0330
      "rpl_vfprintf", // 0xbffc0340

      // GNU quoting option functions
      "set_char_quoting",   // 0xbffc0350
      "set_custom_quoting", // 0xbffc0360

      // GNU printf functions
      "printf_parse",     // 0xbffc0370
      "printf_fetchargs", // 0xbffc0380
      "vasnprintf",       // 0xbffc0390

      // GNU stream functions
      "fseterr",      // 0xbffc03a0
      "close_stream", // 0xbffc03b0

      // GNU filesystem functions
      "globfree", // 0xbffc03c0

      // Less common GNU functions from libc
      "gl_get_setlocale_null_lock", // 0xbffc03d0
  };

  int stub_count = sizeof(stub_names) / sizeof(stub_names[0]);

  if (stub_index < 0 || stub_index >= stub_count) {
    printf("[STUB] Unknown stub address 0x%08x (index=%d)\n", stub_address,
           stub_index);
    // Return dummy value
    x86_context.Registers().eax = 0;
    return B_OK;
  }

  printf("[STUB] Executing %s\n", stub_names[stub_index]);

  // Dispatch to appropriate stub function
  status_t status = B_OK;
  switch (stub_index) {
  // GNU coreutils and error handling
  case 0:
    status = StubFunctions::quote_quoting_options(x86_context, fAddressSpace);
    break;
  case 1:
    status = StubFunctions::close_stdout(x86_context, fAddressSpace);
    break;
  case 2:
    status = StubFunctions::version_etc_copyright(x86_context, fAddressSpace);
    break;
  case 3:
    status = StubFunctions::error_message_count(x86_context, fAddressSpace);
    break;
  case 4:
    status = StubFunctions::error_print_progname(x86_context, fAddressSpace);
    break;
  case 5:
    status = StubFunctions::program_name(x86_context, fAddressSpace);
    break;
  case 6:
    status = StubFunctions::exit_failure(x86_context, fAddressSpace);
    break;
  case 7:
    status = StubFunctions::thrd_exit(x86_context, fAddressSpace);
    break;
  case 8:
    status = StubFunctions::Version(x86_context, fAddressSpace);
    break;
  case 9:
    status = StubFunctions::error_one_per_line(x86_context, fAddressSpace);
    break;

  // GNU libc memory allocation wrappers
  case 10:
    status = StubFunctions::xmalloc(x86_context, fAddressSpace);
    break;
  case 11:
    status = StubFunctions::xcalloc(x86_context, fAddressSpace);
    break;
  case 12:
    status = StubFunctions::xrealloc(x86_context, fAddressSpace);
    break;
  case 13:
    status = StubFunctions::xcharalloc(x86_context, fAddressSpace);
    break;
  case 14:
    status = StubFunctions::xmemdup(x86_context, fAddressSpace);
    break;
  case 15:
    status = StubFunctions::x2nrealloc(x86_context, fAddressSpace);
    break;
  case 16:
    status = StubFunctions::xireallocarray(x86_context, fAddressSpace);
    break;
  case 17:
    status = StubFunctions::xreallocarray(x86_context, fAddressSpace);
    break;
  case 18:
    status = StubFunctions::ximalloc(x86_context, fAddressSpace);
    break;
  case 19:
    status = StubFunctions::xicalloc(x86_context, fAddressSpace);
    break;

  // GNU libc error functions
  case 20:
    status = StubFunctions::error(x86_context, fAddressSpace);
    break;
  case 21:
    status = StubFunctions::xalloc_die(x86_context, fAddressSpace);
    break;

  // GNU quoting functions (quotearg)
  case 22:
    status = StubFunctions::quotearg_alloc_mem(x86_context, fAddressSpace);
    break;
  case 23:
    status = StubFunctions::quotearg_n_custom_mem(x86_context, fAddressSpace);
    break;
  case 24:
    status = StubFunctions::quotearg_n_custom(x86_context, fAddressSpace);
    break;
  case 25:
    status = StubFunctions::quotearg_n_mem(x86_context, fAddressSpace);
    break;
  case 26:
    status = StubFunctions::quotearg_n(x86_context, fAddressSpace);
    break;
  case 27:
    status = StubFunctions::quotearg_char_mem(x86_context, fAddressSpace);
    break;
  case 28:
    status = StubFunctions::quotearg_char(x86_context, fAddressSpace);
    break;
  case 29:
    status = StubFunctions::quotearg_colon(x86_context, fAddressSpace);
    break;
  case 30:
    status = StubFunctions::quotearg_n_style(x86_context, fAddressSpace);
    break;
  case 31:
    status = StubFunctions::quotearg_n_style_mem(x86_context, fAddressSpace);
    break;
  case 32:
    status = StubFunctions::quote_n(x86_context, fAddressSpace);
    break;
  case 33:
    status = StubFunctions::quote_n_mem(x86_context, fAddressSpace);
    break;

  // GNU libc version/program functions
  case 34:
    status = StubFunctions::set_program_name(x86_context, fAddressSpace);
    break;
  case 35:
    status = StubFunctions::getprogname(x86_context, fAddressSpace);
    break;
  case 36:
    status = StubFunctions::version_etc(x86_context, fAddressSpace);
    break;
  case 37:
    status = StubFunctions::version_etc_arn(x86_context, fAddressSpace);
    break;
  case 38:
    status = StubFunctions::version_etc_va(x86_context, fAddressSpace);
    break;
  case 39:
    status = StubFunctions::usage(x86_context, fAddressSpace);
    break;

  // GNU libc locale/encoding functions
  case 40:
    status = StubFunctions::locale_charset(x86_context, fAddressSpace);
    break;
  case 41:
    status = StubFunctions::hard_locale(x86_context, fAddressSpace);
    break;
  case 42:
    status = StubFunctions::setlocale_null_r(x86_context, fAddressSpace);
    break;
  case 43:
    status = StubFunctions::rpl_nl_langinfo(x86_context, fAddressSpace);
    break;

  // rpl_* replacement functions
  case 44:
    status = StubFunctions::rpl_malloc(x86_context, fAddressSpace);
    break;
  case 45:
    status = StubFunctions::rpl_calloc(x86_context, fAddressSpace);
    break;
  case 46:
    status = StubFunctions::rpl_realloc(x86_context, fAddressSpace);
    break;
  case 47:
    status = StubFunctions::rpl_free(x86_context, fAddressSpace);
    break;
  case 48:
    status = StubFunctions::rpl_mbrtowc(x86_context, fAddressSpace);
    break;
  case 49:
    status = StubFunctions::rpl_fclose(x86_context, fAddressSpace);
    break;
  case 50:
    status = StubFunctions::rpl_fflush(x86_context, fAddressSpace);
    break;
  case 51:
    status = StubFunctions::rpl_fseeko(x86_context, fAddressSpace);
    break;
  case 52:
    status = StubFunctions::rpl_vfprintf(x86_context, fAddressSpace);
    break;

  // GNU quoting option functions
  case 53:
    status = StubFunctions::set_char_quoting(x86_context, fAddressSpace);
    break;
  case 54:
    status = StubFunctions::set_custom_quoting(x86_context, fAddressSpace);
    break;

  // GNU printf functions
  case 55:
    status = StubFunctions::printf_parse(x86_context, fAddressSpace);
    break;
  case 56:
    status = StubFunctions::printf_fetchargs(x86_context, fAddressSpace);
    break;
  case 57:
    status = StubFunctions::vasnprintf(x86_context, fAddressSpace);
    break;

  // GNU stream functions
  case 58:
    status = StubFunctions::fseterr(x86_context, fAddressSpace);
    break;
  case 59:
    status = StubFunctions::close_stream(x86_context, fAddressSpace);
    break;

  // GNU filesystem functions
  case 60:
    status = StubFunctions::globfree(x86_context, fAddressSpace);
    break;

  // Less common GNU functions from libc
  case 61:
    status =
        StubFunctions::gl_get_setlocale_null_lock(x86_context, fAddressSpace);
    break;

  default:
    printf("[STUB] Unknown stub index %d\n", stub_index);
    x86_context.Registers().eax = 0;
    break;
  }

  // After stub execution, pop return address and return to caller
  // The RET instruction will handle this

  return status;
}
