#include "OpcodeDatabase.h"
#include <string.h>

#define OP(prim, sec, mnem, type, minb, maxb, modrm, imm8, imm16, imm32, disp8, disp32, rmem, wmem, aflags, ctrlflow) \
	{prim, sec, mnem, type, minb, maxb, modrm, imm8, imm16, imm32, disp8, disp32, rmem, wmem, aflags, ctrlflow}

static const OpcodeInfo kOpcodeTable[] = {
	OP(0x00, 0xFF, "ADD", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x01, 0xFF, "ADD", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x02, 0xFF, "ADD", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x03, 0xFF, "ADD", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x05, 0xFF, "ADD", OpcodeType::ADD, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x04, 0xFF, "ADD", OpcodeType::ADD, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x08, 0xFF, "OR", OpcodeType::OR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x09, 0xFF, "OR", OpcodeType::OR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x0A, 0xFF, "OR", OpcodeType::OR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x0B, 0xFF, "OR", OpcodeType::OR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x0D, 0xFF, "OR", OpcodeType::OR, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x0C, 0xFF, "OR", OpcodeType::OR, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x10, 0xFF, "ADC", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x11, 0xFF, "ADC", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x12, 0xFF, "ADC", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x13, 0xFF, "ADC", OpcodeType::ADD, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x15, 0xFF, "ADC", OpcodeType::ADD, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x14, 0xFF, "ADC", OpcodeType::ADD, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x18, 0xFF, "SBB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x19, 0xFF, "SBB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x1A, 0xFF, "SBB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x1B, 0xFF, "SBB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x1D, 0xFF, "SBB", OpcodeType::SUB, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x1C, 0xFF, "SBB", OpcodeType::SUB, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x20, 0xFF, "AND", OpcodeType::AND, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x21, 0xFF, "AND", OpcodeType::AND, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x22, 0xFF, "AND", OpcodeType::AND, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x23, 0xFF, "AND", OpcodeType::AND, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x25, 0xFF, "AND", OpcodeType::AND, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x24, 0xFF, "AND", OpcodeType::AND, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x28, 0xFF, "SUB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x29, 0xFF, "SUB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x2A, 0xFF, "SUB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x2B, 0xFF, "SUB", OpcodeType::SUB, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x2D, 0xFF, "SUB", OpcodeType::SUB, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x2C, 0xFF, "SUB", OpcodeType::SUB, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x30, 0xFF, "XOR", OpcodeType::XOR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x31, 0xFF, "XOR", OpcodeType::XOR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x32, 0xFF, "XOR", OpcodeType::XOR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x33, 0xFF, "XOR", OpcodeType::XOR, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x35, 0xFF, "XOR", OpcodeType::XOR, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x34, 0xFF, "XOR", OpcodeType::XOR, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x38, 0xFF, "CMP", OpcodeType::CMP, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x39, 0xFF, "CMP", OpcodeType::CMP, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x3A, 0xFF, "CMP", OpcodeType::CMP, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x3B, 0xFF, "CMP", OpcodeType::CMP, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0x3D, 0xFF, "CMP", OpcodeType::CMP, 5, 5, false, false, false, true, false, false, false, false, true, false),
	OP(0x3C, 0xFF, "CMP", OpcodeType::CMP, 2, 2, false, true, false, false, false, false, false, false, true, false),
	
	OP(0x40, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x41, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x42, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x43, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x44, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x45, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x46, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x47, 0xFF, "INC", OpcodeType::INC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	
	OP(0x48, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x49, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x4A, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x4B, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x4C, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x4D, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x4E, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	OP(0x4F, 0xFF, "DEC", OpcodeType::DEC, 1, 1, false, false, false, false, false, false, false, false, true, false),
	
	OP(0x50, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x51, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x52, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x53, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x54, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x55, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x56, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	OP(0x57, 0xFF, "PUSH", OpcodeType::PUSH, 1, 1, false, false, false, false, false, false, false, true, false, false),
	
	OP(0x58, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x59, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x5A, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x5B, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x5C, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x5D, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x5E, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	OP(0x5F, 0xFF, "POP", OpcodeType::POP, 1, 1, false, false, false, false, false, false, true, false, false, false),
	
	OP(0x68, 0xFF, "PUSH", OpcodeType::PUSH, 5, 5, false, false, false, true, false, false, false, true, false, false),
	OP(0x6A, 0xFF, "PUSH", OpcodeType::PUSH, 2, 2, false, true, false, false, false, false, false, true, false, false),
	OP(0x69, 0xFF, "IMUL", OpcodeType::IMUL, 3, 6, true, false, false, true, false, false, true, false, true, false),
	OP(0x6B, 0xFF, "IMUL", OpcodeType::IMUL, 3, 3, true, true, false, false, false, false, true, false, true, false),
	
	OP(0x70, 0xFF, "JO", OpcodeType::JO, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x71, 0xFF, "JNO", OpcodeType::JNO, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x72, 0xFF, "JB", OpcodeType::JB, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x73, 0xFF, "JAE", OpcodeType::JAE, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x74, 0xFF, "JZ", OpcodeType::JZ, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x75, 0xFF, "JNZ", OpcodeType::JNZ, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x76, 0xFF, "JBE", OpcodeType::JBE, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x77, 0xFF, "JA", OpcodeType::JA, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x78, 0xFF, "JS", OpcodeType::JS, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x79, 0xFF, "JNS", OpcodeType::JNS, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x7A, 0xFF, "JP", OpcodeType::JP, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x7B, 0xFF, "JNP", OpcodeType::JNP, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x7C, 0xFF, "JL", OpcodeType::JL, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x7D, 0xFF, "JGE", OpcodeType::JGE, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x7E, 0xFF, "JLE", OpcodeType::JLE, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0x7F, 0xFF, "JG", OpcodeType::JG, 2, 2, false, true, false, false, false, false, false, false, false, true),
	
	OP(0x81, 0xFF, "GRP1", OpcodeType::UNKNOWN, 3, 6, true, false, false, true, false, false, true, false, true, false),
	OP(0x83, 0xFF, "GRP1", OpcodeType::UNKNOWN, 3, 3, true, true, false, false, false, false, true, false, true, false),
	
	OP(0x8B, 0xFF, "MOV", OpcodeType::MOV, 2, 6, true, false, false, false, false, false, true, false, false, false),
	OP(0x89, 0xFF, "MOV", OpcodeType::MOV, 2, 6, true, false, false, false, false, false, true, true, false, false),
	OP(0x8D, 0xFF, "LEA", OpcodeType::LEA, 2, 6, true, false, false, false, false, false, true, false, false, false),
	
	OP(0x90, 0xFF, "NOP", OpcodeType::NOP, 1, 1, false, false, false, false, false, false, false, false, false, false),
	
	OP(0xA1, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, true, false, false, false),
	OP(0xA3, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, true, false, false),
	
	OP(0xA9, 0xFF, "TEST", OpcodeType::TEST, 5, 5, false, false, false, true, false, false, false, false, true, false),
	
	OP(0xB0, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB1, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB2, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB3, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB4, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB5, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB6, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	OP(0xB7, 0xFF, "MOV", OpcodeType::MOV, 2, 2, false, true, false, false, false, false, false, false, false, false),
	
	OP(0xB8, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xB9, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xBA, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xBB, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xBC, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xBD, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xBE, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	OP(0xBF, 0xFF, "MOV", OpcodeType::MOV, 5, 5, false, false, false, true, false, false, false, false, false, false),
	
	OP(0xC1, 0xFF, "GRP2", OpcodeType::UNKNOWN, 3, 3, true, true, false, false, false, false, true, false, true, false),
	OP(0xC3, 0xFF, "RET", OpcodeType::RET, 1, 1, false, false, false, false, false, false, true, false, false, true),
	OP(0xC9, 0xFF, "LEAVE", OpcodeType::LEAVE, 1, 1, false, false, false, false, false, false, true, true, false, false),
	OP(0xC7, 0xFF, "MOV", OpcodeType::MOV, 3, 6, true, false, false, true, false, false, false, true, false, false),
	
	OP(0xCD, 0xFF, "INT", OpcodeType::INT, 2, 2, false, true, false, false, false, false, false, false, false, true),
	
	OP(0xD1, 0xFF, "GRP2", OpcodeType::UNKNOWN, 2, 2, true, false, false, false, false, false, true, false, true, false),
	OP(0xD3, 0xFF, "GRP2", OpcodeType::UNKNOWN, 2, 2, true, false, false, false, false, false, true, false, true, false),
	
	OP(0xE8, 0xFF, "CALL", OpcodeType::CALL, 5, 5, false, false, false, true, false, false, false, true, false, true),
	OP(0xE9, 0xFF, "JMP", OpcodeType::JMP, 5, 5, false, false, false, true, false, false, false, false, false, true),
	OP(0xEB, 0xFF, "JMP", OpcodeType::JMP, 2, 2, false, true, false, false, false, false, false, false, false, true),
	OP(0xFF, 0xFF, "GRP5", OpcodeType::UNKNOWN, 2, 6, true, false, false, false, false, false, true, false, false, true),
};

static constexpr size_t kOpcodeTableSize = sizeof(kOpcodeTable) / sizeof(kOpcodeTable[0]);

const OpcodeInfo* OpcodeDatabase::Lookup(uint8_t opcode) {
	for (size_t i = 0; i < kOpcodeTableSize; ++i) {
		if (kOpcodeTable[i].primary == opcode && kOpcodeTable[i].secondary == 0xFF) {
			return &kOpcodeTable[i];
		}
	}
	return nullptr;
}

const OpcodeInfo* OpcodeDatabase::Lookup(uint8_t primary, uint8_t secondary) {
	for (size_t i = 0; i < kOpcodeTableSize; ++i) {
		if (kOpcodeTable[i].primary == primary && kOpcodeTable[i].secondary == secondary) {
			return &kOpcodeTable[i];
		}
	}
	return nullptr;
}

const char* OpcodeDatabase::GetMnemonic(uint8_t opcode) {
	const OpcodeInfo* info = Lookup(opcode);
	return info ? info->mnemonic : "???";
}

OpcodeType OpcodeDatabase::GetType(uint8_t opcode) {
	const OpcodeInfo* info = Lookup(opcode);
	return info ? info->type : OpcodeType::UNKNOWN;
}

bool OpcodeDatabase::IsSingleByteOpcode(uint8_t opcode) {
	const OpcodeInfo* info = Lookup(opcode);
	return info && info->min_bytes == 1 && info->max_bytes == 1;
}

uint32_t OpcodeDatabase::GetMaxOpcodeSize() {
	uint32_t max_size = 0;
	for (size_t i = 0; i < kOpcodeTableSize; ++i) {
		if (kOpcodeTable[i].max_bytes > max_size) {
			max_size = kOpcodeTable[i].max_bytes;
		}
	}
	return max_size;
}
