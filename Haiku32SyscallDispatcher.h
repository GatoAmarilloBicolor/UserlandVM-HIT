/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>
#include "SyscallDispatcher.h"

class GuestContext;
class AddressSpace;
class X86_32GuestContext;
class HaikuGUISyscalls;

// Implementación del dispatcher de syscalls para Haiku 32-bit
// Traduce syscalls x86-32 de Haiku a syscalls nativas del host
class Haiku32SyscallDispatcher : public SyscallDispatcher {
public:
	Haiku32SyscallDispatcher(AddressSpace* addressSpace = NULL);
	virtual ~Haiku32SyscallDispatcher();

	// Implementa la interfaz SyscallDispatcher
	virtual status_t Dispatch(GuestContext& context) override;

private:
	// Syscalls implementadas
	status_t SyscallWrite(uint32 fd, const void* buffer, uint32 size, uint32& result);
	status_t SyscallExit(int32 code);
	status_t SyscallBrk(uint32 addr, uint32& result);
	status_t SyscallGetCwd(char* buffer, uint32 size, uint32& result);
	status_t SyscallChdir(const char* path, uint32& result);
	
	// Sprint 4: File I/O syscalls
	status_t SyscallOpen(const char* path, int32 flags, int32 mode, uint32& result);
	status_t SyscallClose(uint32 fd, uint32& result);
	status_t SyscallRead(uint32 fd, void* buffer, uint32 size, uint32& result);
	status_t SyscallSeek(uint32 fd, uint32 offset, int32 whence, uint32& result);

	// Mapeo de syscalls x86-32 a implementaciones
	// EAX contiene el número de syscall en x86-32 (Linux i386 ABI / Haiku x86-32)
	static const uint32 SYSCALL_EXIT = 1;
	static const uint32 SYSCALL_WRITE = 4;
	static const uint32 SYSCALL_READ = 3;
	static const uint32 SYSCALL_OPEN = 5;
	static const uint32 SYSCALL_CLOSE = 6;
	static const uint32 SYSCALL_SEEK = 19;  // lseek
	static const uint32 SYSCALL_CHDIR = 12;
	static const uint32 SYSCALL_BRK = 45;
	static const uint32 SYSCALL_GETCWD = 183;
	
	// Haiku GUI syscalls (custom extension range 50000+)
	// These are not Linux syscalls - they're Haiku-specific
	static const uint32 HAIKU_SYSCALL_GUI_BASE = 50000;
	static const uint32 HAIKU_SYSCALL_APP_SERVER_PORT = 50001;
	static const uint32 HAIKU_SYSCALL_REGISTER_WINDOW = 50002;
	static const uint32 HAIKU_SYSCALL_UNREGISTER_WINDOW = 50003;
	static const uint32 HAIKU_SYSCALL_SET_WINDOW_TITLE = 50004;
	static const uint32 HAIKU_SYSCALL_SHOW_WINDOW = 50005;
	static const uint32 HAIKU_SYSCALL_HIDE_WINDOW = 50006;
	static const uint32 HAIKU_SYSCALL_MOVE_WINDOW = 50007;
	static const uint32 HAIKU_SYSCALL_RESIZE_WINDOW = 50008;
	static const uint32 HAIKU_SYSCALL_DESTROY_WINDOW = 50009;
	static const uint32 HAIKU_SYSCALL_FILL_RECT = 50010;
	static const uint32 HAIKU_SYSCALL_DRAW_STRING = 50011;
	static const uint32 HAIKU_SYSCALL_SET_COLOR = 50012;
	static const uint32 HAIKU_SYSCALL_FLUSH_GRAPHICS = 50013;
	static const uint32 HAIKU_SYSCALL_GET_MOUSE_POSITION = 50014;
	static const uint32 HAIKU_SYSCALL_READ_KEYBOARD = 50015;
	static const uint32 HAIKU_SYSCALL_GET_WINDOW_FRAME = 50016;
	static const uint32 HAIKU_SYSCALL_SET_WINDOW_FRAME = 50017;
	static const uint32 HAIKU_SYSCALL_SCREENSHOT = 50018;
	
	// GUI syscall dispatcher
	status_t DispatchGUISyscall(uint32 syscall_num, X86_32GuestContext& context);
	
	// File descriptor tracking
	static const int MAX_FDS = 16;
	int fOpenFds[MAX_FDS];  // Maps guest FDs to host FDs (-1 = unused)

	AddressSpace* fAddressSpace;
};
