#pragma once

#include "Loader.h"

// Execute x86 32-bit binaries on x86 64-bit host
class ExecutionBootstrap {
public:
	ExecutionBootstrap();
	~ExecutionBootstrap();

	// Load and execute x86 32-bit Haiku program
	int ExecuteProgram(const char *programPath, char **argv, char **env);

private:
	struct ProgramContext {
		ElfImage *image;
		void *stackBase;
		size_t stackSize;
		uint32 entryPoint;
		uint32 stackPointer;
	};

	bool SetupX86Environment(ProgramContext &ctx, char **argv, char **env);
	bool BuildX86Stack(ProgramContext &ctx, char **argv, char **env);
	void *AllocateStack(size_t size);

	static const size_t DEFAULT_STACK_SIZE = 0x100000;  // 1MB stack
};

