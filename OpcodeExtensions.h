#pragma once

#include <SupportDefs.h>
#include "GuestContext.h"

class AddressSpace;
class SyscallDispatcher;

class OpcodeExtensions {
public:
	static status_t Execute(GuestContext& context, uint8_t opcode, 
	                        const uint8_t* instr, uint32_t& len,
	                        AddressSpace& space, SyscallDispatcher& dispatcher);
};
