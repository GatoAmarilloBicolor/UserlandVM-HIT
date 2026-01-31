#pragma once

#include <SupportDefs.h>
#include "InterpreterX86_32.h"

class FastInterpreter : public InterpreterX86_32 {
public:
	FastInterpreter(AddressSpace& addressSpace, SyscallDispatcher& dispatcher);
	virtual ~FastInterpreter();

	virtual status_t Run(GuestContext& context) override;

private:
	static const int FAST_CACHE_SIZE = 256;
	
	struct CachedInstr {
		uint8_t opcode;
		uint8_t size;
		uint32_t addr;
	};

	CachedInstr fCache[FAST_CACHE_SIZE];
	uint32_t fCacheIndex;

	inline bool TryFastPath(GuestContext& context, uint8_t opcode, uint32_t& bytes_consumed);
};
