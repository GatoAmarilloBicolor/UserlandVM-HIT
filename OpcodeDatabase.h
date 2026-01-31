#pragma once

#include <stdint.h>

enum class OpcodeType {
	MOV, ADD, SUB, XOR, AND, OR,
	CMP, TEST,
	PUSH, POP,
	CALL, RET, JMP,
	JZ, JNZ, JL, JLE, JG, JGE, JA, JAE, JB, JBE, JP, JNP, JS, JNS, JO, JNO,
	LEA, XCHG,
	INC, DEC,
	IMUL, MUL, IDIV, DIV,
	SHL, SHR, SAR, ROL, ROR,
	INT, LEAVE,
	NOP,
	UNKNOWN
};

struct OpcodeInfo {
	uint8_t primary;
	uint8_t secondary;  // 0xFF if n/a
	const char* mnemonic;
	OpcodeType type;
	
	uint8_t min_bytes;
	uint8_t max_bytes;
	
	bool has_modrm;
	bool has_imm8;
	bool has_imm16;
	bool has_imm32;
	bool has_disp8;
	bool has_disp32;
	
	bool reads_memory;
	bool writes_memory;
	bool affects_flags;
	bool is_control_flow;
};

class OpcodeDatabase {
public:
	static const OpcodeInfo* Lookup(uint8_t opcode);
	static const OpcodeInfo* Lookup(uint8_t primary, uint8_t secondary);
	static const char* GetMnemonic(uint8_t opcode);
	static OpcodeType GetType(uint8_t opcode);
	static bool IsSingleByteOpcode(uint8_t opcode);
	static uint32_t GetMaxOpcodeSize();
};
