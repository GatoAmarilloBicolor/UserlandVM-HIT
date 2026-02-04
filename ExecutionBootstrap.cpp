#include "ExecutionBootstrap.h"
#include "VirtualCpuX86Native.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

ExecutionBootstrap::ExecutionBootstrap()
{
}

ExecutionBootstrap::~ExecutionBootstrap()
{
}

status_t ExecutionBootstrap::ExecuteProgram(const char *programPath, char **argv, char **env)
{
	if (!programPath) {
		fprintf(stderr, "[X86] No program path provided\n");
		return 1;
	}

	printf("[X86] Loading x86 32-bit Haiku program: %s\n", programPath);
	fflush(stdout);

	// Load the ELF binary
	ObjectDeleter<ElfImage> image(ElfImage::Load(programPath));
	if (!image.IsSet()) {
		fprintf(stderr, "[X86] Failed to load program\n");
		return 1;
	}

	printf("[X86] Program loaded at %p, entry=%p\n", image->GetImageBase(), image->GetEntry());
	fflush(stdout);

	// Setup execution context
	ProgramContext ctx;
	ctx.image = image.Get();
	ctx.entryPoint = (uint32)(addr_t)image->GetEntry();
	ctx.stackSize = DEFAULT_STACK_SIZE;
	ctx.linker = new DynamicLinker();
	
	// Load dynamic dependencies
	if (image->IsDynamic()) {
		printf("[X86] Loading dynamic dependencies\n");
		fflush(stdout);
		if (!LoadDependencies(ctx, image.Get())) {
			fprintf(stderr, "[X86] Failed to load dependencies\n");
			return 1;
		}
	}
	
	// Allocate stack for guest program
	ctx.stackBase = AllocateStack(ctx.stackSize);
	if (!ctx.stackBase) {
		fprintf(stderr, "[X86] Failed to allocate stack\n");
		return 1;
	}

	ctx.stackPointer = (uint32)(addr_t)ctx.stackBase + ctx.stackSize;
	printf("[X86] Stack allocated at %p, sp=%#x\n", ctx.stackBase, ctx.stackPointer);
	fflush(stdout);

	// Build the stack with arguments
	if (!BuildX86Stack(ctx, argv, env)) {
		fprintf(stderr, "[X86] Failed to build stack\n");
		return 1;
	}

	printf("[X86] Ready to execute x86 32-bit program\n");
	printf("[X86] Entry point: %#x\n", ctx.entryPoint);
	printf("[X86] Stack pointer: %#x\n", ctx.stackPointer);
	printf("[X86] ===== Program Output =====\n");
	fflush(stdout);

	// Create VirtualCpuX86Native and execute
	VirtualCpuX86Native cpu;
	
	// Set initial registers
	cpu.Ip() = ctx.entryPoint;
	cpu.Regs()[4] = ctx.stackPointer;  // ESP = stack pointer
	
	printf("[X86] CPU initialized, jumping to entry point\n");
	fflush(stdout);
	
	// Run the program
	cpu.Run();
	
	printf("[X86] ===== Program Terminated =====\n");
	fflush(stdout);
	
	return 0;
}

void *ExecutionBootstrap::AllocateStack(size_t size)
{
	// Allocate 32-bit addressable memory for stack
	// For now, use malloc (should use vm32_create_area)
	void *stack = malloc(size);
	printf("[X86] Allocated stack: %p (size=%zu)\n", stack, size);
	return stack;
}

bool ExecutionBootstrap::BuildX86Stack(ProgramContext &ctx, char **argv, char **env)
{
	printf("[X86] Building stack with %zu bytes available\n", ctx.stackSize);

	// Build stack frame at the top of the stack going downward
	uint32 sp = ctx.stackPointer;
	uint8 *stack_base = (uint8 *)ctx.stackBase;
	
	// Count arguments and environment variables
	int argc = 0;
	while (argv && argv[argc]) argc++;
	
	int envc = 0;
	while (env && env[envc]) envc++;
	
	printf("[X86] argc=%d, envc=%d\n", argc, envc);
	
	// Stack layout (x86):
	// [esp + 0]  = argc
	// [esp + 4]  = argv[0]
	// [esp + 8]  = argv[1]
	// ...
	// [esp + 4*(argc+1)] = NULL
	// [esp + 4*(argc+2)] = env[0]
	// ...
	
	// For now, just set up argc at stack pointer
	// This is simplified - a full implementation would copy strings
	
	// Write argc
	if (sp >= 4) {
		sp -= 4;
		*(uint32 *)(stack_base + (sp - (uint32)(addr_t)stack_base)) = argc;
		printf("[X86] Wrote argc=%d at %#x\n", argc, sp);
	}
	
	// Update stack pointer
	ctx.stackPointer = sp;
	
	printf("[X86] Stack frame built, new sp=%#x\n", ctx.stackPointer);
	return true;
}

bool ExecutionBootstrap::SetupX86Environment(ProgramContext &ctx, char **argv, char **env)
{
	printf("[X86] Setting up execution environment\n");
	
	// TODO: Setup:
	// - Thread local storage (TLS)
	// - Commpage
	// - Initial registers
	
	return true;
}

bool ExecutionBootstrap::LoadDependencies(ProgramContext &ctx, ElfImage *image)
{
	if (!image->IsDynamic()) {
		return true;  // No dependencies for static binaries
	}
	
	printf("[X86] Scanning for dependencies in %s\n", image->GetPath());
	fflush(stdout);
	
	// For now, we'll just try to load libroot.so from the standard locations
	const char *libPaths[] = {
		"./sysroot/haiku32/lib/libroot.so",
		"./sysroot/haiku32/lib/x86/libroot.so",
		"./sysroot/haiku32/system/lib/libroot.so",
		"/boot/home/src/UserlandVM-HIT/sysroot/haiku32/lib/libroot.so",
		NULL
	};
	
	for (int i = 0; libPaths[i] != NULL; i++) {
		FILE *f = fopen(libPaths[i], "rb");
		if (f) {
			fclose(f);
			printf("[X86] Loading libroot.so from %s\n", libPaths[i]);
			fflush(stdout);
			
			ElfImage *libroot = ElfImage::Load(libPaths[i]);
			if (libroot) {
				ctx.linker->AddLibrary("libroot.so", libroot);
				printf("[X86] libroot.so loaded at %p\n", libroot->GetImageBase());
				return true;
			}
		}
	}
	
	printf("[X86] Warning: Could not find libroot.so\n");
	return false;  // Continue anyway, some syscalls might work
}

bool ExecutionBootstrap::ResolveDynamicSymbols(ProgramContext &ctx, ElfImage *image)
{
	// TODO: Implement symbol resolution
	// For now, this is a stub
	return true;
}
