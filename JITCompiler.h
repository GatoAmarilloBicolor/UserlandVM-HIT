#pragma once

#include <SupportDefs.h>
#include <stdint.h>
#include <vector>
#include <map>
#include "X86_32GuestContext.h"
#include "AddressSpace.h"

typedef status_t (*CompiledCode)(X86_32Registers*, AddressSpace*, SyscallDispatcher*);

class JITCompiler {
public:
	JITCompiler(AddressSpace& addressSpace);
	~JITCompiler();

	CompiledCode CompileBasicBlock(uint32_t guest_addr, uint32_t max_size = 0x1000);
	void InvalidateCache(uint32_t addr, uint32_t size);
	void ClearCache();
	
	bool IsCompiled(uint32_t addr) const;
	CompiledCode GetCompiled(uint32_t addr) const;

private:
	AddressSpace& fAddressSpace;
	std::map<uint32_t, CompiledCode> fCompiledCache;
	std::vector<uint8_t*> fAllocatedCode;

	struct X86OpcodeMeta {
		uint8_t opcode;
		const char* name;
		uint8_t min_size;
		uint8_t max_size;
		bool reads_memory;
		bool writes_memory;
		bool is_jump;
		bool is_call;
		bool is_syscall;
	};

	static const X86OpcodeMeta kOpcodeMeta[];
	
	bool CanCompile(uint32_t addr, uint32_t max_size, uint32_t& out_size);
	uint8_t* AllocateCode(size_t size);
	void FreeCode(uint8_t* ptr);
};
