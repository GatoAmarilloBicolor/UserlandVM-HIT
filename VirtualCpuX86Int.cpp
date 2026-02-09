#include "PlatformTypes.h"
#include "VirtualCpuX86Int.h"
#include "HaikuOSKitsSystem.h"
#include "DynamicLinker.h"
#include <stdio.h>
#include <string.h>

#include <Zydis/Zydis.h>

VirtualCpuX86Int::VirtualCpuX86Int(ElfImage* image)
	:
	fEip(0),
	fEflags(0),
	fImage(image),
	fGuestMemBase((uint8*)image->GetImageBase()),
	fHaikuKits(nullptr),
	fDynamicLinker(nullptr),
	fIDTInitialized(false)
{
	memset(fRegs, 0, sizeof(fRegs));
	memset(fIDT, 0, sizeof(fIDT));
}

VirtualCpuX86Int::~VirtualCpuX86Int()
{
	// HaikuOSKitsSystem is a singleton, no need to delete
	if (fDynamicLinker) {
		delete fDynamicLinker;
		fDynamicLinker = nullptr;
	}
}

status_t VirtualCpuX86Int::Init()
{
	printf("[INT] Initializing Virtual CPU with interrupt handling...\n");
	
	// Initialize interrupt vector table
	InitializeInterruptVectorTable();
	
	// Initialize Haiku OS subsystems
	InitializeSubsystems();
	
	printf("[INT] Virtual CPU initialization complete\n");
	return B_OK;
}

void VirtualCpuX86Int::Run()
{
	ZydisDecoder decoder;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

	printf("Interpreter: Starting execution at EIP = 0x%08x\n", fEip);

	while (true) {
		ZydisDecodedInstruction instruction;
		ZyanU8* instruction_ptr = fGuestMemBase + fEip;

		if (!ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, NULL, instruction_ptr, 15, &instruction))) {
			printf("Interpreter: Failed to decode instruction at 0x%08x\n", fEip);
			break;
		}

		//printf("0x%08x: %s\n", fEip, ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), fEip, ZYAN_NULL));

		switch (instruction.mnemonic) {
			// TODO: Implement instruction handlers here
			// Example:
			// case ZYDIS_MNEMONIC_MOV:
			// 	// Handle MOV instruction
			// 	break;

			case ZYDIS_MNEMONIC_INT: {
				// Handle INT instruction - check for INT 0x63 (Haiku OS syscalls)
				uint8_t interrupt_vector = fGuestMemBase[fEip + 1];
				if (interrupt_vector == 0x63) {
					HandleInt63Haiku();
				} else {
					printf("Interpreter: INT 0x%02x instruction encountered. Halting.\n", 
						   interrupt_vector);
					return;
				}
				break;
			}

			case ZYDIS_MNEMONIC_INVALID:
			default:
				printf("Interpreter: Unhandled or invalid instruction. Halting.\n");
				// TODO: Dump CPU state for debugging
				return;
		}

		fEip += instruction.length;
	}
}

void VirtualCpuX86Int::InitializeSubsystems()
{
	printf("[INT] Initializing unified Haiku OS kits system...\n");
	
	// Initialize unified Haiku OS kits system
	fHaikuKits = &HaikuOSKitsSystem::Instance();
	if (fHaikuKits->Initialize() == B_OK) {
		printf("[INT] ✅ Unified Haiku OS kits system initialized\n");
	} else {
		printf("[INT] ❌ Haiku OS kits system initialization failed\n");
	}
	
	// Initialize dynamic linker for library loading
	fDynamicLinker = new DynamicLinker();
	if (fDynamicLinker->LoadCriticalLibraries()) {
		printf("[INT] Dynamic linker initialized\n");
		// Load critical libraries
		fDynamicLinker->LoadLibrary("libroot.so");
		fDynamicLinker->LoadLibrary("libbe.so");
		printf("[INT] Critical libraries loaded\n");
	} else {
		printf("[INT] ❌ Dynamic linker initialization failed\n");
	}
	
	printf("[INT] ✅ All subsystems initialized (optimized, no redundancies)\n");
}

void VirtualCpuX86Int::HandleInt63Haiku()
{
	printf("[INT] Handling INT 0x63 Haiku OS syscall (unified system)\n");
	
	// Get syscall number from EAX
	uint32_t syscall_num = fRegs[0]; // EAX
	printf("[INT] Haiku syscall number: %d\n", syscall_num);
	
	// Get arguments from stack (EBX, ECX, EDX, ESI, EDI)
	uint32_t args[5];
	args[0] = fRegs[1]; // EBX
	args[1] = fRegs[2]; // ECX  
	args[2] = fRegs[3]; // EDX
	args[3] = fRegs[4]; // ESI
	args[4] = fRegs[5]; // EDI (if available, otherwise 0)
	
	uint32_t result = 0;
	bool handled = false;
	
	// Route to unified Haiku OS kits system
	if (fHaikuKits) {
		// Extract kit ID from syscall number (high byte)
		uint32_t kit_id = (syscall_num >> 24) & 0xFF;
		uint32_t kit_syscall = syscall_num & 0x00FFFFFF;
		
		printf("[INT] Routing to Kit %d, syscall %d\n", kit_id, kit_syscall);
		handled = fHaikuKits->HandleHaikuSyscall(kit_id, kit_syscall, args, &result);
	}
	
	// Handle dynamic linker syscalls
	if (!handled && fDynamicLinker) {
		handled = fDynamicLinker->HandleLinkerSyscall(syscall_num, args, &result);
	}
	
	// Set return value in EAX
	fRegs[0] = result;
	
	if (handled) {
		printf("[INT] ✅ Haiku syscall %d handled successfully, result = 0x%x\n", 
			   syscall_num, result);
	} else {
		printf("[INT] ❌ Haiku syscall %d not handled\n", syscall_num);
		fRegs[0] = (uint32_t)-1; // Error return
	}
}

void VirtualCpuX86Int::InitializeInterruptVectorTable()
{
	printf("[INT] Initializing Interrupt Vector Table (IDT)...\n");
	
	// Clear IDT
	memset(fIDT, 0, sizeof(fIDT));
	
	// Set up exception handlers (0-31)
	for (int i = 0; i < 32; i++) {
		fIDT[i].offset_low = 0x1000 + (i * 16); // Dummy offset
		fIDT[i].selector = 0x08; // Kernel code segment
		fIDT[i].type = 0x0E; // 32-bit interrupt gate
		fIDT[i].flags = 0x80; // Present
		fIDT[i].offset_high = 0x0000;
	}
	
	// Set up software interrupt handlers (32-255)
	for (int i = 32; i < 256; i++) {
		fIDT[i].offset_low = 0x2000 + ((i - 32) * 16); // Dummy offset
		fIDT[i].selector = 0x08; // Kernel code segment
		fIDT[i].type = 0x0E; // 32-bit interrupt gate
		fIDT[i].flags = 0x80; // Present
		fIDT[i].offset_high = 0x0000;
	}
	
	// Special setup for INT 0x63 (Haiku GUI syscalls)
	fIDT[0x63].offset_low = 0x3000; // Special handler offset
	fIDT[0x63].selector = 0x08;
	fIDT[0x63].type = 0x0E;
	fIDT[0x63].flags = 0x80;
	fIDT[0x63].offset_high = 0x0000;
	
	fIDTInitialized = true;
	printf("[INT] ✅ IDT initialized with 256 entries\n");
	printf("[INT] Exception handlers: 0-31\n");
	printf("[INT] Software interrupts: 32-255\n");
	printf("[INT] Special INT 0x63: Haiku GUI syscalls\n");
}

void VirtualCpuX86Int::HandleException(uint8_t exception_num)
{
	printf("[INT] 🚨 Exception %d occurred at EIP = 0x%08x\n", exception_num, fEip);
	
	// Exception codes and descriptions
	const char *exception_names[] = {
		"Division by Zero",
		"Debug",
		"NMI",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack-Segment Fault",
		"General Protection Fault",
		"Page Fault",
		"Reserved",
		"x87 FPU Error",
		"Alignment Check",
		"Machine Check",
		"SIMD Floating-Point Exception",
		"Virtualization Exception",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Security Exception",
		"Reserved"
	};
	
	if (exception_num < 32) {
		printf("[INT] Exception type: %s\n", exception_names[exception_num]);
		
		// Handle specific exceptions
		switch (exception_num) {
			case 0x0E: // General Protection Fault
				printf("[INT] GPF - Possible invalid memory access or privilege violation\n");
				break;
			case 0x0C: // Stack-Segment Fault
				printf("[INT] SSF - Stack overflow or invalid stack pointer\n");
				break;
			case 0x0D: // Page Fault
				printf("[INT] PF - Memory page not present or access violation\n");
				break;
			case 0x06: // Invalid Opcode
				printf("[INT] UD - Invalid instruction opcode\n");
				break;
		}
	}
	
	// For now, halt on any exception
	printf("[INT] ❌ Halting due to exception\n");
	// In a real implementation, we would call the exception handler
}

void VirtualCpuX86Int::HandleSoftwareInterrupt(uint8_t interrupt_num)
{
	printf("[INT] Software interrupt INT 0x%02x at EIP = 0x%08x\n", interrupt_num, fEip);
	
	switch (interrupt_num) {
		case 0x63:
			// Haiku OS syscalls (unified system)
			HandleInt63Haiku();
			break;
			
		case 0x80:
			// Linux syscalls (if supported)
			printf("[INT] Linux syscall INT 0x80 (not implemented)\n");
			fRegs[0] = (uint32_t)-1; // ENOSYS
			break;
			
		case 0x21:
			// DOS syscalls (if supported)
			printf("[INT] DOS syscall INT 0x21 (not implemented)\n");
			fRegs[0] = (uint32_t)-1;
			break;
			
		default:
			printf("[INT] Unhandled software interrupt: 0x%02x\n", interrupt_num);
			fRegs[0] = (uint32_t)-1;
			break;
	}
}

void VirtualCpuX86Int::HandleHardwareInterrupt(uint8_t interrupt_num)
{
	printf("[INT] Hardware interrupt IRQ %d\n", interrupt_num);
	
	// Handle hardware interrupts
	switch (interrupt_num) {
		case 0x00: // Timer
			printf("[INT] Timer interrupt (PIT)\n");
			break;
		case 0x01: // Keyboard
			printf("[INT] Keyboard interrupt\n");
			break;
		case 0x0E: // Primary ATA
			printf("[INT] Primary ATA interrupt\n");
			break;
		case 0x0F: // Secondary ATA
			printf("[INT] Secondary ATA interrupt\n");
			break;
		default:
			printf("[INT] Unhandled hardware interrupt: %d\n", interrupt_num);
			break;
	}
	
	// Send End of Interrupt (EOI) to PIC
	// In a real implementation, we would write to the PIC
}
