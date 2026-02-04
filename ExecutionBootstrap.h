#pragma once

#include "Loader.h"
#include "DynamicLinker.h"
#include "RelocationProcessor.h"
#include "DirectAddressSpace.h"
#include <OS.h>

// Execute x86 32-bit binaries on x86 64-bit host
class ExecutionBootstrap {
public:
	ExecutionBootstrap();
	~ExecutionBootstrap();

	// Load and execute x86 32-bit Haiku program
	status_t ExecuteProgram(const char *programPath, char **argv, char **env);

private:
	struct ProgramContext {
		ElfImage *image;
		void *stackBase;
		size_t stackSize;
		uint32 entryPoint;
		uint32 stackPointer;
		DynamicLinker *linker;  // For loading dependencies
	};

	bool SetupX86Environment(ProgramContext &ctx, char **argv, char **env, DirectAddressSpace &addressSpace);
	bool BuildX86Stack(ProgramContext &ctx, char **argv, char **env);
	void *AllocateStack(size_t size);
	bool LoadDependencies(ProgramContext &ctx, ElfImage *image);
	bool ResolveDynamicSymbols(ProgramContext &ctx, ElfImage *image);

	static const size_t DEFAULT_STACK_SIZE = 0x100000;  // 1MB stack
};

