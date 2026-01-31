#include "VirtualCpuX86Int.h"
#include <stdio.h>
#include <string.h>

#include <Zydis/Zydis.h>

VirtualCpuX86Int::VirtualCpuX86Int(ElfImage* image)
	:
	fEip(0),
	fEflags(0),
	fImage(image),
	fGuestMemBase((uint8*)image->GetImageBase())
{
	memset(fRegs, 0, sizeof(fRegs));
}

VirtualCpuX86Int::~VirtualCpuX86Int()
{
}

status_t VirtualCpuX86Int::Init()
{
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
				// For now, we assume INT is a syscall trap and we stop.
				printf("Interpreter: INT instruction encountered. Halting.\n");
				return;
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
