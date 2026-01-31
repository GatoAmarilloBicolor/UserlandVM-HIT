#pragma once

#include <SupportDefs.h>
#include "X86_32GuestContext.h"
#include "AddressSpace.h"

class SyscallDispatcher;

class OptimizedX86Executor {
public:
	OptimizedX86Executor(AddressSpace& space, SyscallDispatcher& disp);
	~OptimizedX86Executor();
	
	status_t Execute(X86_32GuestContext& ctx, uint32_t& bytes_consumed);
	
	static const uint32_t SUPPORTED_OPCODES_COUNT;
	
private:
	AddressSpace& space;
	SyscallDispatcher& dispatcher;
	
	uint8_t cache_buffer[15];
	
	struct Handler {
		uint8_t opcode;
		status_t (OptimizedX86Executor::*func)(X86_32GuestContext&, uint32_t&);
	};
	
	static const Handler handlers[];
	
	status_t LoadInstr(uint32_t addr, uint8_t* buf, uint32_t len);
	
	#define HANDLER_DECL(name) status_t Handle_##name(X86_32GuestContext& ctx, uint32_t& len)
	
	HANDLER_DECL(MOV_rm32_r32);    // 0x89
	HANDLER_DECL(MOV_r32_rm32);    // 0x8B
	HANDLER_DECL(MOV_r32_imm32);   // 0xB8-0xBF
	HANDLER_DECL(MOV_r8_imm8);     // 0xB0-0xB7
	HANDLER_DECL(MOV_rm32_imm32);  // 0xC7
	HANDLER_DECL(LEA_r32_m);       // 0x8D
	
	HANDLER_DECL(ADD_rm32_r32);    // 0x01
	HANDLER_DECL(ADD_r32_rm32);    // 0x03
	HANDLER_DECL(ADD_EAX_imm32);   // 0x05
	HANDLER_DECL(SUB_rm32_r32);    // 0x29
	HANDLER_DECL(SUB_r32_rm32);    // 0x2B
	HANDLER_DECL(SUB_EAX_imm32);   // 0x2D
	HANDLER_DECL(XOR_rm32_r32);    // 0x31
	HANDLER_DECL(XOR_r32_rm32);    // 0x33
	HANDLER_DECL(XOR_EAX_imm32);   // 0x35
	HANDLER_DECL(AND_rm32_r32);    // 0x21
	HANDLER_DECL(AND_r32_rm32);    // 0x23
	HANDLER_DECL(AND_EAX_imm32);   // 0x25
	HANDLER_DECL(OR_rm32_r32);     // 0x09
	HANDLER_DECL(OR_r32_rm32);     // 0x0B
	HANDLER_DECL(OR_EAX_imm32);    // 0x0D
	HANDLER_DECL(CMP_rm32_r32);    // 0x39
	HANDLER_DECL(CMP_r32_rm32);    // 0x3B
	HANDLER_DECL(CMP_EAX_imm32);   // 0x3D
	HANDLER_DECL(TEST_rm32_r32);   // 0x85
	HANDLER_DECL(TEST_EAX_imm32);  // 0xA9
	
	HANDLER_DECL(PUSH_r32);        // 0x50-0x57
	HANDLER_DECL(POP_r32);         // 0x58-0x5F
	HANDLER_DECL(PUSH_imm32);      // 0x68
	HANDLER_DECL(PUSH_imm8);       // 0x6A
	
	HANDLER_DECL(CALL_rel32);      // 0xE8
	HANDLER_DECL(RET);             // 0xC3
	HANDLER_DECL(LEAVE);           // 0xC9
	HANDLER_DECL(JMP_rel32);       // 0xE9
	HANDLER_DECL(JMP_rel8);        // 0xEB
	
	HANDLER_DECL(JZ_rel8);         // 0x74
	HANDLER_DECL(JNZ_rel8);        // 0x75
	HANDLER_DECL(JL_rel8);         // 0x7C
	HANDLER_DECL(JGE_rel8);        // 0x7D
	HANDLER_DECL(JLE_rel8);        // 0x7E
	HANDLER_DECL(JG_rel8);         // 0x7F
	HANDLER_DECL(JB_rel8);         // 0x72
	HANDLER_DECL(JAE_rel8);        // 0x73
	HANDLER_DECL(JBE_rel8);        // 0x76
	HANDLER_DECL(JA_rel8);         // 0x77
	HANDLER_DECL(JO_rel8);         // 0x70
	HANDLER_DECL(JNO_rel8);        // 0x71
	HANDLER_DECL(JP_rel8);         // 0x7A
	HANDLER_DECL(JNP_rel8);        // 0x7B
	HANDLER_DECL(JS_rel8);         // 0x78
	HANDLER_DECL(JNS_rel8);        // 0x79
	
	HANDLER_DECL(INC_r32);         // 0x40-0x47
	HANDLER_DECL(DEC_r32);         // 0x48-0x4F
	
	HANDLER_DECL(NOP);             // 0x90
	HANDLER_DECL(INT);             // 0xCD
	HANDLER_DECL(IMUL);            // 0x69, 0x6B
	
	HANDLER_DECL(XCHG_r32_r32);    // 0x87
	HANDLER_DECL(SHL_r32_imm8);    // 0xC1 /4
	HANDLER_DECL(SHR_r32_imm8);    // 0xC1 /5
	HANDLER_DECL(SAR_r32_imm8);    // 0xC1 /7
	HANDLER_DECL(SHL_r32_CL);      // 0xD3 /4
	HANDLER_DECL(SHR_r32_CL);      // 0xD3 /5
	HANDLER_DECL(SAR_r32_CL);      // 0xD3 /7
	HANDLER_DECL(ROL_r32_imm8);    // 0xC1 /0
	HANDLER_DECL(ROR_r32_imm8);    // 0xC1 /1
	HANDLER_DECL(ROL_r32_CL);      // 0xD3 /0
	HANDLER_DECL(ROR_r32_CL);      // 0xD3 /1
	
	HANDLER_DECL(NEG_r32);         // 0xF7 /3
	HANDLER_DECL(NOT_r32);         // 0xF7 /2
	HANDLER_DECL(MUL_r32);         // 0xF7 /4
	HANDLER_DECL(IMUL_r32);        // 0xF7 /5
	HANDLER_DECL(DIV_r32);         // 0xF7 /6
	HANDLER_DECL(IDIV_r32);        // 0xF7 /7
	
	HANDLER_DECL(MOVSXD_r32_rm32); // 0x63 (32-bit variant)
	HANDLER_DECL(MOVSX_r32_rm8);   // 0xBE (sign extend byte)
	HANDLER_DECL(MOVSX_r32_rm16);  // 0xBF (sign extend word)
	HANDLER_DECL(MOVZX_r32_rm8);   // 0xB6 (zero extend byte)
	HANDLER_DECL(MOVZX_r32_rm16);  // 0xB7 (zero extend word)
	
	HANDLER_DECL(CBWDQ);           // 0x98 (sign extend EAX)
	HANDLER_DECL(CWDCDQ);          // 0x99 (extend EDX:EAX)
	
	HANDLER_DECL(ADC_r32_rm32);    // 0x13
	HANDLER_DECL(ADC_EAX_imm32);   // 0x15
	HANDLER_DECL(SBB_r32_rm32);    // 0x1B
	HANDLER_DECL(SBB_EAX_imm32);   // 0x1D
	
	HANDLER_DECL(Unsupported);
};

#undef HANDLER_DECL
