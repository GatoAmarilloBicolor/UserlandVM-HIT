/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>
#include "ExecutionEngine.h"
#include "X86_32GuestContext.h"

class GuestContext;
class SyscallDispatcher;
class AddressSpace;
class OptimizedX86Executor;

// ModR/M Decoder Helper Struct
// Holds decoded information from ModR/M byte and any displacement bytes
struct ModRM {
	uint8 mod;              // 0-3 (addressing mode)
	uint8 reg_op;           // 0-7 (register field or opcode extension)
	uint8 rm;               // 0-7 (register or memory reference)
	int32 displacement;     // Displacement value (-128 to +2GB)
	uint8 bytes_used;       // Total bytes consumed (1-6)
};

// Intérprete simple para x86-32
// Decodifica y ejecuta instrucciones x86 una por una
class InterpreterX86_32 : public ExecutionEngine {
public:
	InterpreterX86_32(AddressSpace& addressSpace, SyscallDispatcher& dispatcher);
	virtual ~InterpreterX86_32();

	virtual status_t Run(GuestContext& context) override;

private:
	AddressSpace& fAddressSpace;
	SyscallDispatcher& fDispatcher;
	OptimizedX86Executor* fOptimizedExecutor;

	// Límite de instrucciones para evitar loops infinitos en debugging
	static const uint32 MAX_INSTRUCTIONS = 10000000;  // 10 million instructions

	// Decodificación y ejecución de instrucciones
	status_t ExecuteInstruction(GuestContext& context, uint32& bytes_consumed);

	// Helper functions for ModR/M addressing (Sprint 4)
	status_t DecodeModRM(const uint8* instr, ModRM& result);
	uint32 GetEffectiveAddress(X86_32Registers& regs, const ModRM& modrm);

	// Instrucciones soportadas
	status_t Execute_MOV(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_MOV_Load(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_MOV_Load_FS(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_MOV_Store(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_INT(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_PUSH(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_PUSH_Imm(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_POP(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_ADD(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_SUB(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_CMP(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_XOR(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JMP(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_RET(GuestContext& context, const uint8* instr, uint32& len);
	
	// Sprint 4: Conditional jumps
	status_t Execute_JZ(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JNZ(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JL(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JLE(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JG(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JGE(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JA(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JAE(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JB(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JBE(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JP(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JNP(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JS(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JNS(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JO(GuestContext& context, const uint8* instr, uint32& len);
	status_t Execute_JNO(GuestContext& context, const uint8* instr, uint32& len);

	// Handling common group opcodes (e.g., 0x83 for ADD, SUB, CMP with 8-bit immediate)
	status_t Execute_GROUP_83(GuestContext& context, const uint8* instr, uint32& len);
	
	// GROUP 81 (0x81) - ADD, SUB, CMP with 32-bit immediate
	status_t Execute_GROUP_81(GuestContext& context, const uint8* instr, uint32& len);
	
	// GROUP C1/D1/D3 (0xC1) - Shift/Rotate operations (SHL, SHR, SAR, ROL, ROR)
	status_t Execute_GROUP_C1(GuestContext& context, const uint8* instr, uint32& len);

	// Helper for setting EFLAGS after ADD operations
	template<typename T>
	void SetFlags_ADD(X86_32Registers& regs, T result, T op1, T op2, bool is_32bit);

	// Sprint 5: Dynamic linking support
	status_t Execute_CALL(GuestContext& context, const uint8* instr, uint32& len);
	
	// Stub function dispatcher
	status_t ExecuteStubFunction(GuestContext& context, uint32_t stub_address);
};
