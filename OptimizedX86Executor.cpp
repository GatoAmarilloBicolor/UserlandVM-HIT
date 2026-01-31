#include "OptimizedX86Executor.h"
#include "X86_32GuestContext.h"
#include "AddressSpace.h"
#include "SyscallDispatcher.h"
#include <cstring>
#include <cstdio>

OptimizedX86Executor::OptimizedX86Executor(AddressSpace& space, SyscallDispatcher& disp)
	: space(space), dispatcher(disp) {}

OptimizedX86Executor::~OptimizedX86Executor() {}

const OptimizedX86Executor::Handler OptimizedX86Executor::handlers[] = {
	{0x89, &OptimizedX86Executor::Handle_MOV_rm32_r32},
	{0x8B, &OptimizedX86Executor::Handle_MOV_r32_rm32},
	{0x8D, &OptimizedX86Executor::Handle_LEA_r32_m},
	{0xC7, &OptimizedX86Executor::Handle_MOV_rm32_imm32},
	
	{0x01, &OptimizedX86Executor::Handle_ADD_rm32_r32},
	{0x03, &OptimizedX86Executor::Handle_ADD_r32_rm32},
	{0x05, &OptimizedX86Executor::Handle_ADD_EAX_imm32},
	{0x29, &OptimizedX86Executor::Handle_SUB_rm32_r32},
	{0x2B, &OptimizedX86Executor::Handle_SUB_r32_rm32},
	{0x2D, &OptimizedX86Executor::Handle_SUB_EAX_imm32},
	{0x31, &OptimizedX86Executor::Handle_XOR_rm32_r32},
	{0x33, &OptimizedX86Executor::Handle_XOR_r32_rm32},
	{0x35, &OptimizedX86Executor::Handle_XOR_EAX_imm32},
	{0x21, &OptimizedX86Executor::Handle_AND_rm32_r32},
	{0x23, &OptimizedX86Executor::Handle_AND_r32_rm32},
	{0x25, &OptimizedX86Executor::Handle_AND_EAX_imm32},
	{0x09, &OptimizedX86Executor::Handle_OR_rm32_r32},
	{0x0B, &OptimizedX86Executor::Handle_OR_r32_rm32},
	{0x0D, &OptimizedX86Executor::Handle_OR_EAX_imm32},
	{0x39, &OptimizedX86Executor::Handle_CMP_rm32_r32},
	{0x3B, &OptimizedX86Executor::Handle_CMP_r32_rm32},
	{0x3D, &OptimizedX86Executor::Handle_CMP_EAX_imm32},
	{0x85, &OptimizedX86Executor::Handle_TEST_rm32_r32},
	{0xA9, &OptimizedX86Executor::Handle_TEST_EAX_imm32},
	
	{0xC3, &OptimizedX86Executor::Handle_RET},
	{0xC9, &OptimizedX86Executor::Handle_LEAVE},
	{0xE8, &OptimizedX86Executor::Handle_CALL_rel32},
	{0xE9, &OptimizedX86Executor::Handle_JMP_rel32},
	{0xEB, &OptimizedX86Executor::Handle_JMP_rel8},
	
	{0x74, &OptimizedX86Executor::Handle_JZ_rel8},
	{0x75, &OptimizedX86Executor::Handle_JNZ_rel8},
	{0x7C, &OptimizedX86Executor::Handle_JL_rel8},
	{0x7D, &OptimizedX86Executor::Handle_JGE_rel8},
	{0x7E, &OptimizedX86Executor::Handle_JLE_rel8},
	{0x7F, &OptimizedX86Executor::Handle_JG_rel8},
	{0x72, &OptimizedX86Executor::Handle_JB_rel8},
	{0x73, &OptimizedX86Executor::Handle_JAE_rel8},
	{0x76, &OptimizedX86Executor::Handle_JBE_rel8},
	{0x77, &OptimizedX86Executor::Handle_JA_rel8},
	{0x70, &OptimizedX86Executor::Handle_JO_rel8},
	{0x71, &OptimizedX86Executor::Handle_JNO_rel8},
	{0x7A, &OptimizedX86Executor::Handle_JP_rel8},
	{0x7B, &OptimizedX86Executor::Handle_JNP_rel8},
	{0x78, &OptimizedX86Executor::Handle_JS_rel8},
	{0x79, &OptimizedX86Executor::Handle_JNS_rel8},
	
	{0x90, &OptimizedX86Executor::Handle_NOP},
	{0xCD, &OptimizedX86Executor::Handle_INT},
	
	{0x87, &OptimizedX86Executor::Handle_XCHG_r32_r32},
	{0xC1, &OptimizedX86Executor::Handle_SHL_r32_imm8},
	{0xD3, &OptimizedX86Executor::Handle_SHL_r32_CL},
	{0x98, &OptimizedX86Executor::Handle_CBWDQ},
	{0x99, &OptimizedX86Executor::Handle_CWDCDQ},
	{0x13, &OptimizedX86Executor::Handle_ADC_r32_rm32},
	{0x15, &OptimizedX86Executor::Handle_ADC_EAX_imm32},
	{0x1B, &OptimizedX86Executor::Handle_SBB_r32_rm32},
	{0x1D, &OptimizedX86Executor::Handle_SBB_EAX_imm32},
};

status_t OptimizedX86Executor::LoadInstr(uint32_t addr, uint8_t* buf, uint32_t len) {
	return space.Read(addr, buf, len);
}

status_t OptimizedX86Executor::Execute(X86_32GuestContext& ctx, uint32_t& bytes_consumed) {
	X86_32Registers& regs = ctx.Registers();
	
	if (LoadInstr(regs.eip, cache_buffer, 15) != B_OK) {
		return B_ERROR;
	}
	
	uint8_t opcode = cache_buffer[0];
	
	for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
		if (handlers[i].opcode == opcode) {
			return (this->*handlers[i].func)(ctx, bytes_consumed);
		}
	}
	
	return Handle_Unsupported(ctx, bytes_consumed);
}

status_t OptimizedX86Executor::Handle_MOV_rm32_r32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	
	if (mod == 3) {
		*regs[rm] = *regs[reg];
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_MOV_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	
	if (mod == 3) {
		*regs[reg] = *regs[rm];
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_LEA_r32_m(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	
	if (mod == 1) {
		int8_t disp8 = (int8_t)cache_buffer[2];
		*regs[reg] = *regs[rm] + disp8;
		len = 3;
		return B_OK;
	} else if (mod == 2) {
		int32_t disp32 = *(int32_t*)&cache_buffer[2];
		*regs[reg] = *regs[rm] + disp32;
		len = 6;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_MOV_r32_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t opcode = cache_buffer[0];
	int reg = opcode - 0xB8;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	*regs[reg] = *(uint32_t*)&cache_buffer[1];
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOV_r8_imm8(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t opcode = cache_buffer[0];
	int reg = opcode - 0xB0;
	uint8_t imm8 = cache_buffer[1];
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	*regs[reg] = (*regs[reg] & 0xFFFFFF00) | imm8;
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOV_rm32_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t rm = modrm & 7;
	
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	
	if (mod == 3) {
		*regs[rm] = *(uint32_t*)&cache_buffer[2];
		len = 6;
		return B_OK;
	}
	return B_BAD_DATA;
}

#define BINOP(name, op) \
status_t OptimizedX86Executor::Handle_##name(X86_32GuestContext& ctx, uint32_t& len) { \
	X86_32Registers& r = ctx.Registers(); \
	uint8_t modrm = cache_buffer[1]; \
	uint8_t mod = (modrm >> 6) & 3; \
	uint8_t reg = (modrm >> 3) & 7; \
	uint8_t rm = modrm & 7; \
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi}; \
	if (mod == 3) { \
		uint32_t res = *regs[rm] op *regs[reg]; \
		*regs[rm] = res; \
		r.eflags &= ~(0x40 | 0x80 | 1); \
		if (res == 0) r.eflags |= 0x40; \
		if ((int32_t)res < 0) r.eflags |= 0x80; \
		len = 2; \
		return B_OK; \
	} \
	return B_BAD_DATA; \
}

BINOP(ADD_rm32_r32, +)
BINOP(SUB_rm32_r32, -)
BINOP(XOR_rm32_r32, ^)
BINOP(AND_rm32_r32, &)
BINOP(OR_rm32_r32, |)

status_t OptimizedX86Executor::Handle_ADD_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[reg] + *regs[rm];
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_ADD_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	r.eax += imm32;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_SUB_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[reg] - *regs[rm];
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_SUB_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	r.eax -= imm32;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_XOR_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[reg] ^ *regs[rm];
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_XOR_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	r.eax ^= imm32;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_AND_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[reg] & *regs[rm];
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_AND_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	r.eax &= imm32;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_OR_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[reg] | *regs[rm];
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_OR_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	r.eax |= imm32;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_CMP_rm32_r32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[rm] - *regs[reg];
		r.eflags &= ~(0x40 | 0x80 | 1);
		if (res == 0) r.eflags |= 0x40;
		if ((int32_t)res < 0) r.eflags |= 0x80;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_CMP_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[reg] - *regs[rm];
		r.eflags &= ~(0x40 | 0x80 | 1);
		if (res == 0) r.eflags |= 0x40;
		if ((int32_t)res < 0) r.eflags |= 0x80;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_CMP_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	uint32_t res = r.eax - imm32;
	r.eflags &= ~(0x40 | 0x80 | 1);
	if (res == 0) r.eflags |= 0x40;
	if ((int32_t)res < 0) r.eflags |= 0x80;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_TEST_rm32_r32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t res = *regs[rm] & *regs[reg];
		r.eflags &= ~(0x40 | 0x80 | 1);
		if (res == 0) r.eflags |= 0x40;
		if ((int32_t)res < 0) r.eflags |= 0x80;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_TEST_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	uint32_t res = r.eax & imm32;
	r.eflags &= ~(0x40 | 0x80 | 1);
	if (res == 0) r.eflags |= 0x40;
	if ((int32_t)res < 0) r.eflags |= 0x80;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_RET(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	space.Read(r.esp, &r.eip, 4);
	r.esp += 4;
	len = 0;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_LEAVE(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	r.esp = r.ebp;
	space.Read(r.esp, &r.ebp, 4);
	r.esp += 4;
	len = 1;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_CALL_rel32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	int32_t offset = *(int32_t*)&cache_buffer[1];
	uint32_t ret_addr = r.eip + 5;
	r.esp -= 4;
	space.Write(r.esp, &ret_addr, 4);
	r.eip = r.eip + 5 + offset;
	len = 0;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_JMP_rel32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	int32_t offset = *(int32_t*)&cache_buffer[1];
	r.eip = r.eip + 5 + offset;
	len = 0;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_JMP_rel8(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	int8_t offset = (int8_t)cache_buffer[1];
	r.eip = r.eip + 2 + offset;
	len = 0;
	return B_OK;
}

#define CJUMP(name, cond) \
status_t OptimizedX86Executor::Handle_##name(X86_32GuestContext& ctx, uint32_t& len) { \
	X86_32Registers& r = ctx.Registers(); \
	int8_t offset = (int8_t)cache_buffer[1]; \
	if (cond) { \
		r.eip = r.eip + 2 + offset; \
		len = 0; \
	} else { \
		len = 2; \
	} \
	return B_OK; \
}

CJUMP(JZ_rel8, (r.eflags & 0x40) != 0)
CJUMP(JNZ_rel8, (r.eflags & 0x40) == 0)
CJUMP(JL_rel8, (((r.eflags >> 7) & 1) != ((r.eflags >> 11) & 1)))
CJUMP(JGE_rel8, (((r.eflags >> 7) & 1) == ((r.eflags >> 11) & 1)))
CJUMP(JLE_rel8, ((r.eflags & 0x40) != 0 || (((r.eflags >> 7) & 1) != ((r.eflags >> 11) & 1))))
CJUMP(JG_rel8, ((r.eflags & 0x40) == 0 && (((r.eflags >> 7) & 1) == ((r.eflags >> 11) & 1))))
CJUMP(JB_rel8, (r.eflags & 1) != 0)
CJUMP(JAE_rel8, (r.eflags & 1) == 0)
CJUMP(JBE_rel8, ((r.eflags & 1) != 0 || (r.eflags & 0x40) != 0))
CJUMP(JA_rel8, ((r.eflags & 1) == 0 && (r.eflags & 0x40) == 0))
CJUMP(JO_rel8, (r.eflags & 0x800) != 0)
CJUMP(JNO_rel8, (r.eflags & 0x800) == 0)
CJUMP(JP_rel8, (r.eflags & 4) != 0)
CJUMP(JNP_rel8, (r.eflags & 4) == 0)
CJUMP(JS_rel8, (r.eflags & 0x80) != 0)
CJUMP(JNS_rel8, (r.eflags & 0x80) == 0)

status_t OptimizedX86Executor::Handle_NOP(X86_32GuestContext& ctx, uint32_t& len) {
	len = 1;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_INT(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_XCHG_r32_r32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t tmp = *regs[reg];
		*regs[reg] = *regs[rm];
		*regs[rm] = tmp;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_SHL_r32_imm8(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint8_t imm8 = cache_buffer[2];
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (reg == 4) *regs[rm] <<= imm8;
	else if (reg == 5) *regs[rm] >>= imm8;
	else if (reg == 7) *regs[rm] = ((int32_t)*regs[rm]) >> imm8;
	len = 3;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_SHR_r32_imm8(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_imm8(ctx, len);
}

status_t OptimizedX86Executor::Handle_SAR_r32_imm8(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_imm8(ctx, len);
}

status_t OptimizedX86Executor::Handle_SHL_r32_CL(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint8_t cl = (uint8_t)r.ecx;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (reg == 4) *regs[rm] <<= cl;
	else if (reg == 5) *regs[rm] >>= cl;
	else if (reg == 7) *regs[rm] = ((int32_t)*regs[rm]) >> cl;
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_SHR_r32_CL(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_CL(ctx, len);
}

status_t OptimizedX86Executor::Handle_SAR_r32_CL(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_CL(ctx, len);
}

status_t OptimizedX86Executor::Handle_ROL_r32_imm8(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_imm8(ctx, len);
}

status_t OptimizedX86Executor::Handle_ROR_r32_imm8(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_imm8(ctx, len);
}

status_t OptimizedX86Executor::Handle_ROL_r32_CL(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_CL(ctx, len);
}

status_t OptimizedX86Executor::Handle_ROR_r32_CL(X86_32GuestContext& ctx, uint32_t& len) {
	return Handle_SHL_r32_CL(ctx, len);
}

status_t OptimizedX86Executor::Handle_NEG_r32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_NOT_r32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MUL_r32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_IMUL_r32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_DIV_r32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_IDIV_r32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOVSXD_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	len = 2;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOVSX_r32_rm8(X86_32GuestContext& ctx, uint32_t& len) {
	len = 3;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOVSX_r32_rm16(X86_32GuestContext& ctx, uint32_t& len) {
	len = 3;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOVZX_r32_rm8(X86_32GuestContext& ctx, uint32_t& len) {
	len = 3;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_MOVZX_r32_rm16(X86_32GuestContext& ctx, uint32_t& len) {
	len = 3;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_CBWDQ(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	if ((int16_t)r.eax < 0) r.eax |= 0xFFFF0000;
	else r.eax &= 0x0000FFFF;
	len = 1;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_CWDCDQ(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	if ((int32_t)r.eax < 0) r.edx = 0xFFFFFFFF;
	else r.edx = 0;
	len = 1;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_ADC_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t carry = r.eflags & 1;
		uint32_t res = *regs[reg] + *regs[rm] + carry;
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_ADC_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	uint32_t carry = r.eflags & 1;
	r.eax = r.eax + imm32 + carry;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_SBB_r32_rm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint8_t modrm = cache_buffer[1];
	uint8_t mod = (modrm >> 6) & 3;
	uint8_t reg = (modrm >> 3) & 7;
	uint8_t rm = modrm & 7;
	uint32_t* regs[] = {&r.eax, &r.ecx, &r.edx, &r.ebx, &r.esp, &r.ebp, &r.esi, &r.edi};
	if (mod == 3) {
		uint32_t carry = r.eflags & 1;
		uint32_t res = *regs[reg] - *regs[rm] - carry;
		*regs[reg] = res;
		len = 2;
		return B_OK;
	}
	return B_BAD_DATA;
}

status_t OptimizedX86Executor::Handle_SBB_EAX_imm32(X86_32GuestContext& ctx, uint32_t& len) {
	X86_32Registers& r = ctx.Registers();
	uint32_t imm32 = *(uint32_t*)&cache_buffer[1];
	uint32_t carry = r.eflags & 1;
	r.eax = r.eax - imm32 - carry;
	len = 5;
	return B_OK;
}

status_t OptimizedX86Executor::Handle_Unsupported(X86_32GuestContext& ctx, uint32_t& len) {
	len = 1;
	return B_BAD_DATA;
}

const uint32_t OptimizedX86Executor::SUPPORTED_OPCODES_COUNT = sizeof(handlers) / sizeof(handlers[0]);
