#ifndef VIRTUAL_CPU_X86_INT_H
#define VIRTUAL_CPU_X86_INT_H

#include <SupportDefs.h>
#include "Loader.h"

class VirtualCpuX86Int {
public:
	VirtualCpuX86Int(ElfImage* image);
	~VirtualCpuX86Int();

	status_t Init();
	void Run();

	uint32& Ip() { return fEip; }
	uint32* Regs() { return fRegs; }

private:
	// General purpose registers: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
	uint32 fRegs[8];
	uint32 fEip; // Instruction Pointer
	uint32 fEflags; // Flags register

	ElfImage* fImage;
	uint8* fGuestMemBase;
};

#endif // VIRTUAL_CPU_X86_INT_H
