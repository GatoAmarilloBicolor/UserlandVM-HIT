#pragma once

#include <SupportDefs.h>
#include "X86_32GuestContext.h"
#include "AddressSpace.h"

class SyscallDispatcher;

class UnifiedExecutor {
public:
	UnifiedExecutor(AddressSpace& space, SyscallDispatcher& dispatcher);
	~UnifiedExecutor();

	status_t ExecuteOneInstruction(X86_32GuestContext& ctx, uint32_t& bytes_consumed);
	
	static const uint32_t ALL_SUPPORTED_OPCODES;
	static bool IsSupported(uint8_t opcode);

private:
	AddressSpace& fSpace;
	SyscallDispatcher& fDispatcher;

	uint8_t instr_buffer[15];
	
	typedef status_t (UnifiedExecutor::*OpcodeHandler)(X86_32GuestContext&, uint32_t&);
	static OpcodeHandler handlers[256];
	static bool handlers_initialized;
	static void InitializeHandlers();
	
	status_t HandleUnsupported(X86_32GuestContext& ctx, uint32_t& len);
	status_t HandleNOP(X86_32GuestContext& ctx, uint32_t& len);
	
	status_t ExecuteMOV(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteADD(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteSUB(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteXOR(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteAND(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteOR(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteCMP(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteTEST(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecutePUSH(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecutePOP(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteCALL(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteRET(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteJMP(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteConditionalJump(X86_32GuestContext& ctx, uint8_t condition, uint32_t& len);
	status_t ExecuteLEA(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteXCHG(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteINC(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteDEC(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteIMUL(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteINT(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteLEAVE(X86_32GuestContext& ctx, uint32_t& len);
	status_t ExecuteShifts(X86_32GuestContext& ctx, uint8_t opcode, uint32_t& len);
	
	uint32_t GetEffectiveAddress(X86_32Registers& regs, uint8_t modrm_byte, const uint8_t* rest, uint32_t& modrm_size);
	void SetZeroFlag(X86_32Registers& regs, uint32_t value);
	void SetSignFlag(X86_32Registers& regs, uint32_t value);
};
