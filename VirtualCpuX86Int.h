#ifndef VIRTUAL_CPU_X86_INT_H
#define VIRTUAL_CPU_X86_INT_H

#include <SupportDefs.h>
#include "Loader.h"

// Forward declarations  
class SmartHaikuEmulation;
class DynamicLinker;

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

	// Haiku OS subsystems
	SmartHaikuEmulation* fSmartEmulation;
	DynamicLinker* fDynamicLinker;

	// INT 0x63 Haiku OS syscall handling
	void HandleInt63Haiku();
	void InitializeSubsystems();
	
	// Interrupt vector table and exception handling
	struct InterruptDescriptor {
		uint32_t offset_low;
		uint16_t selector;
		uint8_t type;
		uint8_t flags;
		uint16_t offset_high;
	};
	
	void InitializeInterruptVectorTable();
	void HandleException(uint8_t exception_num);
	void HandleSoftwareInterrupt(uint8_t interrupt_num);
	void HandleHardwareInterrupt(uint8_t interrupt_num);
	
	// Interrupt descriptor table (IDT)
	InterruptDescriptor fIDT[256];
	bool fIDTInitialized;
};

#endif // VIRTUAL_CPU_X86_INT_H
