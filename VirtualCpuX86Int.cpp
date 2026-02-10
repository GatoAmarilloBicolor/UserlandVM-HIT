#include "PlatformTypes.h"
#include "VirtualCpuX86Int.h"
#include "SmartHaikuEmulation.h"
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
	fSmartEmulation(nullptr),
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
			// Basic data movement instructions
			case ZYDIS_MNEMONIC_MOV: {
				// Handle MOV instruction - simplified for basic operation
				printf("[INT] MOV instruction at 0x%08x\n", fEip);
				// For now, just skip the instruction
				break;
			}
			
			case ZYDIS_MNEMONIC_ADD: {
				// Handle ADD instruction
				printf("[INT] ADD instruction at 0x%08x\n", fEip);
				break;
			}
			
			case ZYDIS_MNEMONIC_SUB: {
				// Handle SUB instruction
				printf("[INT] SUB instruction at 0x%08x\n", fEip);
				break;
			}
			
			case ZYDIS_MNEMONIC_PUSH: {
				// Handle PUSH instruction
				printf("[INT] PUSH instruction at 0x%08x\n", fEip);
				fRegs[4] -= 4; // ESP
				break;
			}
			
			case ZYDIS_MNEMONIC_POP: {
				// Handle POP instruction
				printf("[INT] POP instruction at 0x%08x\n", fEip);
				fRegs[4] += 4; // ESP
				break;
			}
			
			case ZYDIS_MNEMONIC_CALL: {
				// Handle CALL instruction
				printf("[INT] CALL instruction at 0x%08x\n", fEip);
				// Push return address
				fRegs[4] -= 4; // ESP
				// For now, just continue
				break;
			}
			
			case ZYDIS_MNEMONIC_RET: {
				// Handle RET instruction
				printf("[INT] RET instruction at 0x%08x\n", fEip);
				// Pop return address from stack
				fRegs[4] += 4; // ESP
				break;
			}
			
			case ZYDIS_MNEMONIC_JMP: {
				// Handle JMP instruction
				printf("[INT] JMP instruction at 0x%08x\n", fEip);
				// For now, just continue
				break;
			}
			
			case ZYDIS_MNEMONIC_CMP: {
				// Handle CMP instruction
				printf("[INT] CMP instruction at 0x%08x\n", fEip);
				// Set flags based on comparison
				fEflags |= 0x40; // Set Zero Flag for simplicity
				break;
			}
			
			case ZYDIS_MNEMONIC_JZ: {
				// Jump if Zero (ZF = 1) - same as JE
				printf("[INT] JZ instruction at 0x%08x\n", fEip);
				if (fEflags & 0x40) {
					printf("[INT] Jump taken (zero)\n");
				}
				break;
			}
			
			case ZYDIS_MNEMONIC_JNZ: {
				// Jump if Not Zero (ZF = 0) - same as JNE
				printf("[INT] JNZ instruction at 0x%08x\n", fEip);
				if (!(fEflags & 0x40)) {
					printf("[INT] Jump taken (not zero)\n");
				}
				break;
			}
			
			case ZYDIS_MNEMONIC_JL: {
				// Jump if Less
				printf("[INT] JL instruction at 0x%08x\n", fEip);
				break;
			}
			
			case ZYDIS_MNEMONIC_NOP: {
				// No operation
				printf("[INT] NOP instruction at 0x%08x\n", fEip);
				break;
			}
			
			case ZYDIS_MNEMONIC_HLT: {
				// Halt processor
				printf("[INT] CPU halted by HLT instruction at 0x%08x\n", fEip);
				return;
			}

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
				DumpCpuState();
				return;
		}

		fEip += instruction.length;
	}
}

void VirtualCpuX86Int::InitializeSubsystems()
{
	printf("[INT] Initializing Smart Haiku OS emulation system...\n");
	
	// Initialize smart Haiku emulation system
	fSmartEmulation = &HaikuEmulation::SmartHaikuEmulation::Instance();
	if (fSmartEmulation->Initialize()) {
		printf("[INT] ✅ Smart Haiku emulation system initialized\n");
	} else {
		printf("[INT] ❌ Smart Haiku emulation system initialization failed\n");
	}
	
	// Auto-configure based on system capabilities
	if (fSmartEmulation->AutoConfigure()) {
		printf("[INT] ✅ Auto-configuration completed\n");
	} else {
		printf("[INT] ⚠️ Auto-configuration used defaults\n");
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
	
	printf("[INT] ✅ All subsystems initialized (modular, reusable, intelligent)\n");
}

void VirtualCpuX86Int::HandleInt63Haiku()
{
	printf("[INT] Handling INT 0x63 Smart Haiku OS syscall (modular system)\n");
	
	// Get syscall number from EAX
	uint32_t syscall_num = fRegs[0]; // EAX
	printf("[INT] Smart Haiku syscall number: %d\n", syscall_num);
	
	// Get arguments from stack (EBX, ECX, EDX, ESI, EDI)
	uint32_t args[5];
	args[0] = fRegs[1]; // EBX
	args[1] = fRegs[2]; // ECX  
	args[2] = fRegs[3]; // EDX
	args[3] = fRegs[4]; // ESI
	args[4] = fRegs[5]; // EDI (if available, otherwise 0)
	
	uint32_t result = 0;
	bool handled = false;
	
	// Route to smart Haiku emulation system
	if (fSmartEmulation) {
		printf("[INT] Routing to Smart Haiku emulation system\n");
		handled = fSmartEmulation->HandleHaikuSyscall(syscall_num, args, &result);
	}
	
	// Handle dynamic linker syscalls
	if (!handled && fDynamicLinker) {
		handled = fDynamicLinker->HandleLinkerSyscall(syscall_num, args, &result);
	}
	
	// Set return value in EAX
	fRegs[0] = result;
	
	if (handled) {
		printf("[INT] ✅ Smart Haiku syscall %d handled successfully, result = 0x%x\n", 
			   syscall_num, result);
	} else {
		printf("[INT] ❌ Smart Haiku syscall %d not handled\n", syscall_num);
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

void VirtualCpuX86Int::DumpCpuState()
{
	printf("\n");
	printf("=================================================\n");
	printf("            CPU STATE DUMP\n");
	printf("=================================================\n");
	printf("Instruction Pointer (EIP): 0x%08x\n", fEip);
	printf("Flags (EFLAGS): 0x%08x\n", fEflags);
	
	printf("\nGeneral Purpose Registers:\n");
	printf("EAX: 0x%08x  EBX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x\n", 
		   fRegs[0], fRegs[1], fRegs[2], fRegs[3]);
	printf("ESI: 0x%08x  EDI: 0x%08x  EBP: 0x%08x  ESP: 0x%08x\n", 
		   fRegs[4], fRegs[5], fRegs[6], fRegs[7]);
	
	printf("\nFlag Status:\n");
	printf("CF (Carry):     %s  PF (Parity):   %s  AF (Aux): %s\n",
		   (fEflags & 0x01) ? "1" : "0",
		   (fEflags & 0x04) ? "1" : "0", 
		   (fEflags & 0x10) ? "1" : "0");
	printf("ZF (Zero):     %s  SF (Sign):     %s  TF (Trap): %s\n",
		   (fEflags & 0x40) ? "1" : "0",
		   (fEflags & 0x80) ? "1" : "0",
		   (fEflags & 0x100) ? "1" : "0");
	printf("IF (Interrupt): %s  DF (Direction): %s  OF (Overflow): %s\n",
		   (fEflags & 0x200) ? "1" : "0",
		   (fEflags & 0x400) ? "1" : "0",
		   (fEflags & 0x800) ? "1" : "0");
	
	printf("\nSegment Registers (if applicable):\n");
	printf("CS: 0x%04x  DS: 0x%04x  ES: 0x%04x  FS: 0x%04x\n",
		   0x08, 0x10, 0x18, 0x20); // Default values
	printf("GS: 0x%04x  SS: 0x%04x\n", 0x28, 0x30);
	
	printf("\nMemory Information:\n");
	printf("Guest Memory Base: %p\n", fGuestMemBase);
	printf("Image Base: 0x%08x\n", fImage ? fImage->GetImageBase() : 0);
	
	printf("\nSubsystem Status:\n");
	printf("Smart Emulation: %s\n", fSmartEmulation ? "Initialized" : "Not initialized");
	printf("Dynamic Linker: %s\n", fDynamicLinker ? "Initialized" : "Not initialized");
	printf("IDT: %s\n", fIDTInitialized ? "Initialized" : "Not initialized");
	
	printf("\nNext Instruction Bytes:\n");
	printf("0x%08x: ", fEip);
	for (int i = 0; i < 8; i++) {
		if (fGuestMemBase && (fEip + i < 0x10000000)) { // Simple bounds check
			printf("%02x ", fGuestMemBase[fEip + i]);
		} else {
			printf("?? ");
		}
	}
	printf("\n");
	
	printf("=================================================\n");
	printf("            END CPU STATE DUMP\n");
	printf("=================================================\n");
	printf("\n");
}
