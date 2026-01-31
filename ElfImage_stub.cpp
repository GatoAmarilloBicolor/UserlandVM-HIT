/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

// Sprint 2 stubs - implemented methods without using header classes
// to avoid conflicts and allow compilation to complete

// Stub function implementations for linking
extern "C" {
	int vm32_create_area_stub(const char *name, void **address, unsigned int addr_spec,
		unsigned long size, unsigned int lock, unsigned int protection) {
		return -1;  // Error code
	}
}

// Virtual CPU stubs
extern "C" {
	void VirtualCpuX86NativeSwitch32(void *state) {}
	void VirtualCpuX86NativeLoadContext() {}
	void VirtualCpuX86NativeSaveContext() {}
	void VirtualCpuX86NativeReturn() {}
}

// ElfImage stub
extern "C" {
	typedef struct {
		const char *path;
	} ElfImage_t;
	
	int ElfImage_Load(const char* path) {
		return 0;  // Success
	}
}
