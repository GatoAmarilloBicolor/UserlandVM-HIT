#pragma once

#include <SupportDefs.h>
#include <stdint.h>

struct X86Reg {
	uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
	uint32_t eflags, eip;
	
	inline uint32_t& GetReg(int idx) {
		uint32_t* regs[] = {&eax, &ecx, &edx, &ebx, &esp, &ebp, &esi, &edi};
		return *regs[idx & 7];
	}
	
	inline uint16_t GetRegWord(int idx) {
		return (uint16_t)(GetReg(idx));
	}
	
	inline uint8_t GetRegByte(int idx) {
		if (idx < 4) return (uint8_t)(GetReg(idx));
		return (uint8_t)(GetReg(idx - 4) >> 8);
	}
};

#define CF (1 << 0)
#define PF (1 << 2)
#define AF (1 << 4)
#define ZF (1 << 6)
#define SF (1 << 7)
#define OF (1 << 11)

enum class ModRMType {
	REG, REGMEM, MEM
};

struct ModRM {
	uint8_t mod, reg, rm;
	int32_t disp;
	uint8_t size;
	ModRMType type;
};

inline ModRM DecodeModRM(const uint8_t* instr, uint32_t& offset) {
	ModRM result = {};
	uint8_t byte = instr[offset];
	result.mod = (byte >> 6) & 3;
	result.reg = (byte >> 3) & 7;
	result.rm = byte & 7;
	result.size = 1;
	offset++;
	
	if (result.mod == 3) {
		result.type = ModRMType::REG;
	} else if (result.mod == 0 && result.rm == 5) {
		result.disp = *(int32_t*)&instr[offset];
		result.size += 4;
		offset += 4;
		result.type = ModRMType::MEM;
	} else if (result.mod == 1) {
		result.disp = (int8_t)instr[offset];
		result.size += 1;
		offset++;
		result.type = ModRMType::REGMEM;
	} else if (result.mod == 2) {
		result.disp = *(int32_t*)&instr[offset];
		result.size += 4;
		offset += 4;
		result.type = ModRMType::REGMEM;
	} else {
		result.type = ModRMType::MEM;
	}
	return result;
}

class ExecutionCore {
public:
	static inline void SetZeroFlag(X86Reg& r, uint32_t val) {
		if (val == 0) r.eflags |= ZF;
		else r.eflags &= ~ZF;
	}
	
	static inline void SetSignFlag(X86Reg& r, uint32_t val) {
		if (val & 0x80000000) r.eflags |= SF;
		else r.eflags &= ~SF;
	}
	
	static inline void SetCarryFlag(X86Reg& r, bool carry) {
		if (carry) r.eflags |= CF;
		else r.eflags &= ~CF;
	}
	
	static inline void SetOverflowFlag(X86Reg& r, uint32_t dst, uint32_t src, uint32_t result) {
		bool overflow = ((dst ^ result) & (src ^ result)) & 0x80000000;
		if (overflow) r.eflags |= OF;
		else r.eflags &= ~OF;
	}
	
	static inline void UpdateFlags_ADD(X86Reg& r, uint32_t dst, uint32_t src, uint64_t result) {
		SetCarryFlag(r, result > 0xFFFFFFFFULL);
		uint32_t res32 = (uint32_t)result;
		SetZeroFlag(r, res32);
		SetSignFlag(r, res32);
		SetOverflowFlag(r, dst, src, res32);
	}
	
	static inline void UpdateFlags_SUB(X86Reg& r, uint32_t dst, uint32_t src, uint32_t result) {
		SetCarryFlag(r, dst < src);
		SetZeroFlag(r, result);
		SetSignFlag(r, result);
		SetOverflowFlag(r, dst, ~src, result);
	}
};
