/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "Haiku32SyscallDispatcher.h"
#include "GuestContext.h"
#include "X86_32GuestContext.h"
#include "AddressSpace.h"
#include "DebugOutput.h"

Haiku32SyscallDispatcher::Haiku32SyscallDispatcher(AddressSpace* addressSpace)
	: fAddressSpace(addressSpace)
{
	// Initialize file descriptor table
	for (int i = 0; i < MAX_FDS; i++) {
		fOpenFds[i] = -1;
	}
	// Pre-populate standard file descriptors
	fOpenFds[0] = 0;  // stdin
	fOpenFds[1] = 1;  // stdout
	fOpenFds[2] = 2;  // stderr
}

Haiku32SyscallDispatcher::~Haiku32SyscallDispatcher()
{
}

status_t Haiku32SyscallDispatcher::Dispatch(GuestContext& context)
{
	X86_32GuestContext& x86_context = static_cast<X86_32GuestContext&>(context);
	X86_32Registers& regs = x86_context.Registers();

	uint32 syscall_num = regs.eax;
	uint32 result = 0;
	status_t status = B_OK;

	DebugPrintf("[SYSCALL] dispatch: EAX=%u ", syscall_num);

	// Los primeros 6 argumentos en x86-32 se pasan en registros
	// EBX, ECX, EDX, ESI, EDI, EBP (en ese orden)
	uint32 arg1 = regs.ebx;
	uint32 arg2 = regs.ecx;
	uint32 arg3 = regs.edx;
	(void)regs.esi;
	(void)regs.edi;
	(void)regs.ebp;

	// Check if this is a Haiku GUI syscall
	if (syscall_num >= HAIKU_SYSCALL_GUI_BASE) {
		DispatchGUISyscall(syscall_num, x86_context);
		return B_OK;
	}

	switch (syscall_num) {
		case SYSCALL_EXIT:
			DebugPrintf("exit(%u)\n", arg1);
			status = SyscallExit(arg1);
			break;

		case SYSCALL_WRITE:
			DebugPrintf("write(fd=%u, buf=0x%08x, size=%u)\n", arg1, arg2, arg3);
			status = SyscallWrite(arg1, reinterpret_cast<const void*>(arg2), arg3, result);
			break;

		case SYSCALL_BRK:
			DebugPrintf("brk(addr=0x%08x)\n", arg1);
			status = SyscallBrk(arg1, result);
			break;

		case SYSCALL_GETCWD:
			DebugPrintf("getcwd(buf=0x%08x, size=%u)\n", arg1, arg2);
			status = SyscallGetCwd(reinterpret_cast<char*>(arg1), arg2, result);
			break;

		case SYSCALL_CHDIR:
			DebugPrintf("chdir(path=0x%08x)\n", arg1);
			status = SyscallChdir(reinterpret_cast<const char*>(arg1), result);
			break;

		// Sprint 4: File I/O syscalls
		case SYSCALL_OPEN:
			DebugPrintf("open(path=0x%08x, flags=%d, mode=%d)\n", arg1, arg2, arg3);
			status = SyscallOpen(reinterpret_cast<const char*>(arg1), arg2, arg3, result);
			break;

		case SYSCALL_CLOSE:
			DebugPrintf("close(fd=%u)\n", arg1);
			status = SyscallClose(arg1, result);
			break;

		case SYSCALL_READ:
			DebugPrintf("read(fd=%u, buf=0x%08x, size=%u)\n", arg1, arg2, arg3);
			status = SyscallRead(arg1, reinterpret_cast<void*>(arg2), arg3, result);
			break;

		case SYSCALL_SEEK:
			DebugPrintf("seek(fd=%u, offset=%u, whence=%d)\n", arg1, arg2, arg3);
			status = SyscallSeek(arg1, arg2, arg3, result);
			break;

		default:
			DebugPrintf("UNIMPLEMENTED\n");
			fprintf(stderr, "[SYSCALL] ERROR: Syscall %u not implemented\n", syscall_num);
			result = -38; // ENOSYS
			break;
	}

	// Almacenar el resultado en EAX
	// En x86-32, los valores negativos indican errores, positivos son exitosos
	if (status == B_OK) {
		regs.eax = result;
	} else {
		// Errores se retornan como negativos en x86-32
		regs.eax = (uint32)(-status);
	}

	DebugPrintf("  → EAX=%u (status=%d)\n", regs.eax, status);
	
	// Check if we should exit
	if (syscall_num == SYSCALL_EXIT) {
		context.SetExit(true);
		return (status_t)0x80000001;  // Special exit code
	}
	
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallExit(int32 code)
{
	DebugPrintf("[SYSCALL] Exiting guest with code %d\n", code);
	// Signal to interpreter that guest should exit
	return B_OK;  // Will be handled by interpreter checking context flag
}

status_t Haiku32SyscallDispatcher::SyscallWrite(uint32 fd, const void* buffer,
	uint32 size, uint32& result)
{
	printf("[SYSCALL_WRITE] fd=%d, buffer=0x%08x, size=%u\n", fd, (unsigned int)(uintptr_t)buffer, size);
	
	if (!buffer || size == 0) {
		printf("[SYSCALL_WRITE] Empty buffer or size=0, returning 0\n");
		result = 0;
		return B_OK;
	}

	// Use guest virtual address to read from guest memory
	if (!fAddressSpace) {
		printf("[SYSCALL_WRITE] ERROR: AddressSpace not set\n");
		result = 0;
		return B_BAD_VALUE;
	}

	uint32_t guest_vaddr = (uint32_t)(uintptr_t)buffer;  // Treat as guest virtual address
	
	printf("[SYSCALL_WRITE] Reading %u bytes from guest vaddr=0x%08x\n", size, guest_vaddr);
	
	// Read data from guest memory (using virtual address)
	uint8_t temp_buffer[4096];  // Temporary host buffer
	uint32_t to_read = (size > sizeof(temp_buffer)) ? sizeof(temp_buffer) : size;
	status_t status = fAddressSpace->Read(guest_vaddr, temp_buffer, to_read);
	if (status != B_OK) {
		printf("[SYSCALL_WRITE] ERROR: Failed to read guest memory at vaddr 0x%08x, status=%d\n", guest_vaddr, status);
		result = 0;
		// Don't fail on read error - write 0 bytes
		result = 0;
		return B_OK;
	}

	printf("[SYSCALL_WRITE] Successfully read %u bytes from 0x%08x\n", to_read, guest_vaddr);
	
	// Mapear FDs del guest a FDs del host
	// 1 = stdout, 2 = stderr
	int host_fd = (fd == 1) ? STDOUT_FILENO : (fd == 2) ? STDERR_FILENO : fd;

	printf("[SYSCALL_WRITE] Writing to fd=%d: ", host_fd);
	ssize_t bytes_written = write(host_fd, temp_buffer, to_read);
	printf("\n[SYSCALL_WRITE] Wrote %ld bytes\n", bytes_written);
	fflush(stdout);
	fflush(stderr);

	result = (bytes_written > 0) ? bytes_written : 0;
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallBrk(uint32 addr, uint32& result)
{
	// brk() establece el final del segmento de datos
	// Para ahora, simplemente retornamos el addr actual
	// Una implementación real mantendría track del heap
	DebugPrintf("[SYSCALL] brk: returning current heap end\n");
	result = addr > 0 ? addr : 0x08048000; // Default heap start
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallGetCwd(char* buffer, uint32 size, uint32& result)
{
	if (!buffer || size == 0) {
		return B_BAD_VALUE;
	}

	char cwd[256];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		result = 0;
		return B_IO_ERROR;
	}

	uint32 cwd_len = strlen(cwd) + 1;
	if (cwd_len > size) {
		result = 0;
		return B_BUFFER_OVERFLOW;
	}

	// Copiar a guest memory space
	status_t write_status = fAddressSpace->Write(reinterpret_cast<uintptr_t>(buffer), cwd, cwd_len);
	if (write_status != B_OK) {
		result = 0;
		return write_status;
	}
	result = cwd_len;
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallChdir(const char* path, uint32& result)
{
	if (!path) {
		return B_BAD_VALUE;
	}

	// Traducir path desde guest memory space
	char path_buffer[256];
	uint32_t guest_vaddr = reinterpret_cast<uintptr_t>(path);
	
	status_t read_status = fAddressSpace->ReadString(guest_vaddr, path_buffer, sizeof(path_buffer));
	if (read_status != B_OK) {
		DebugPrintf("[SYSCALL] ERROR: Failed to read path from guest memory at 0x%08x\n", guest_vaddr);
		result = -1;
		return read_status;
	}

	if (chdir(path_buffer) < 0) {
		result = -1;
		return B_IO_ERROR;
	}

	result = 0;
	return B_OK;
}

// Sprint 4: File I/O syscall implementations

status_t Haiku32SyscallDispatcher::SyscallOpen(const char* path, int32 flags, int32 mode, uint32& result)
{
	if (!path || !fAddressSpace) {
		result = -1;
		return B_BAD_VALUE;
	}

	// Translate guest virtual address to host offset
	uint32_t guest_offset = fAddressSpace->TranslateAddress(reinterpret_cast<uintptr_t>(path));
	
	// Read path string from guest memory
	char path_buffer[256];
	uint32_t max_path = sizeof(path_buffer) - 1;
	
	// Read path character by character until null terminator
	for (uint32_t i = 0; i < max_path; i++) {
		uint8_t ch;
		status_t status = fAddressSpace->Read(guest_offset + i, &ch, 1);
		if (status != B_OK) {
			DebugPrintf("[SYSCALL] ERROR: Failed to read path from guest memory\n");
			result = -1;
			return status;
		}
		path_buffer[i] = ch;
		if (ch == '\0') break;
	}
	path_buffer[max_path] = '\0';

	DebugPrintf("[SYSCALL] open: translating guest path at 0x%p to '%s'\n", path, path_buffer);

	// Map Linux open flags to host flags
	int host_flags = 0;
	if (flags & 0) host_flags |= O_RDONLY;
	if (flags & 1) host_flags |= O_WRONLY;
	if (flags & 2) host_flags |= O_RDWR;
	if (flags & 0x8) host_flags |= O_APPEND;
	if (flags & 0x40) host_flags |= O_CREAT;
	if (flags & 0x80) host_flags |= O_EXCL;
	if (flags & 0x200) host_flags |= O_TRUNC;

	// Find a free guest FD slot
	int guest_fd = -1;
	for (int i = 3; i < MAX_FDS; i++) {
		if (fOpenFds[i] == -1) {
			guest_fd = i;
			break;
		}
	}

	if (guest_fd == -1) {
		DebugPrintf("[SYSCALL] ERROR: Too many open files\n");
		result = -1;
		return B_FILE_ERROR;
	}

	// Open file on host
	int host_fd = ::open(path_buffer, host_flags, mode);
	if (host_fd < 0) {
		DebugPrintf("[SYSCALL] ERROR: Failed to open file: %s\n", path_buffer);
		result = -1;
		return B_FILE_ERROR;
	}

	// Map guest FD to host FD
	fOpenFds[guest_fd] = host_fd;
	result = guest_fd;

	DebugPrintf("[SYSCALL] open: opened file as guest_fd=%d (host_fd=%d)\n", guest_fd, host_fd);
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallClose(uint32 fd, uint32& result)
{
	if (fd < 3 || fd >= MAX_FDS) {
		DebugPrintf("[SYSCALL] ERROR: Invalid FD %u\n", fd);
		result = -1;
		return B_BAD_VALUE;
	}

	int host_fd = fOpenFds[fd];
	if (host_fd == -1) {
		DebugPrintf("[SYSCALL] ERROR: FD %u not open\n", fd);
		result = -1;
		return B_FILE_ERROR;
	}

	if (::close(host_fd) < 0) {
		DebugPrintf("[SYSCALL] ERROR: Failed to close FD\n");
		result = -1;
		return B_FILE_ERROR;
	}

	fOpenFds[fd] = -1;
	result = 0;

	DebugPrintf("[SYSCALL] close: closed guest_fd=%u (host_fd=%d)\n", fd, host_fd);
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallRead(uint32 fd, void* buffer, uint32 size, uint32& result)
{
	if (!buffer || size == 0 || !fAddressSpace) {
		result = 0;
		return B_OK;
	}

	if (fd >= MAX_FDS) {
		DebugPrintf("[SYSCALL] ERROR: Invalid FD %u\n", fd);
		result = -1;
		return B_BAD_VALUE;
	}

	int host_fd = fOpenFds[fd];
	if (host_fd == -1 && fd != 0) {  // Allow stdin
		DebugPrintf("[SYSCALL] ERROR: FD %u not open\n", fd);
		result = -1;
		return B_FILE_ERROR;
	}

	// Translate guest buffer address
	uint32_t guest_offset = fAddressSpace->TranslateAddress(reinterpret_cast<uintptr_t>(buffer));

	// Read from host
	uint8_t temp_buffer[4096];
	uint32_t to_read = (size > sizeof(temp_buffer)) ? sizeof(temp_buffer) : size;
	
	ssize_t bytes_read = ::read(host_fd, temp_buffer, to_read);
	if (bytes_read < 0) {
		DebugPrintf("[SYSCALL] ERROR: Failed to read from FD %u\n", fd);
		result = -1;
		return B_IO_ERROR;
	}

	// Write data to guest memory
	status_t status = fAddressSpace->Write(guest_offset, temp_buffer, bytes_read);
	if (status != B_OK) {
		DebugPrintf("[SYSCALL] ERROR: Failed to write to guest memory\n");
		result = -1;
		return status;
	}

	result = bytes_read;
	DebugPrintf("[SYSCALL] read: read %ld bytes from fd=%u into guest_buffer at 0x%p\n", bytes_read, fd, buffer);
	return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallSeek(uint32 fd, uint32 offset, int32 whence, uint32& result)
{
	if (fd >= MAX_FDS) {
		DebugPrintf("[SYSCALL] ERROR: Invalid FD %u\n", fd);
		result = -1;
		return B_BAD_VALUE;
	}

	int host_fd = fOpenFds[fd];
	if (host_fd == -1) {
		DebugPrintf("[SYSCALL] ERROR: FD %u not open\n", fd);
		result = -1;
		return B_FILE_ERROR;
	}

	// Map Linux seek constants to host constants
	int host_whence = SEEK_SET;
	if (whence == 1) host_whence = SEEK_CUR;
	else if (whence == 2) host_whence = SEEK_END;

	off_t new_offset = ::lseek(host_fd, offset, host_whence);
	if (new_offset < 0) {
		DebugPrintf("[SYSCALL] ERROR: Failed to seek in FD %u\n", fd);
		result = -1;
		return B_IO_ERROR;
	}

	result = new_offset;
	DebugPrintf("[SYSCALL] seek: seeked to offset %u in fd=%u\n", (uint32)new_offset, fd);
	return B_OK;
}

// ============================================================================
// GUI Syscall Dispatcher
// ============================================================================

#include "HaikuGUISyscalls.h"

status_t Haiku32SyscallDispatcher::DispatchGUISyscall(uint32 syscall_num, X86_32GuestContext& context)
{
	switch (syscall_num) {
		case HAIKU_SYSCALL_APP_SERVER_PORT:
			return HaikuGUISyscalls::app_server_port(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_REGISTER_WINDOW:
			return HaikuGUISyscalls::register_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_UNREGISTER_WINDOW:
			return HaikuGUISyscalls::unregister_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_SET_WINDOW_TITLE:
			return HaikuGUISyscalls::set_window_title(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_SHOW_WINDOW:
			return HaikuGUISyscalls::show_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_HIDE_WINDOW:
			return HaikuGUISyscalls::hide_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_MOVE_WINDOW:
			return HaikuGUISyscalls::move_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_RESIZE_WINDOW:
			return HaikuGUISyscalls::resize_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_DESTROY_WINDOW:
			return HaikuGUISyscalls::destroy_window(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_FILL_RECT:
			return HaikuGUISyscalls::fill_rect(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_DRAW_STRING:
			return HaikuGUISyscalls::draw_string(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_SET_COLOR:
			return HaikuGUISyscalls::set_color(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_FLUSH_GRAPHICS:
			return HaikuGUISyscalls::flush_graphics(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_GET_MOUSE_POSITION:
			return HaikuGUISyscalls::get_mouse_position(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_READ_KEYBOARD:
			return HaikuGUISyscalls::read_keyboard_input(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_GET_WINDOW_FRAME:
			return HaikuGUISyscalls::get_window_frame(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_SET_WINDOW_FRAME:
			return HaikuGUISyscalls::set_window_frame(context, *fAddressSpace);
		
		case HAIKU_SYSCALL_SCREENSHOT:
			return HaikuGUISyscalls::screenshot(context, *fAddressSpace);
		
		default:
			DebugPrintf("[GUI_SYSCALL] Unknown GUI syscall: %u\n", syscall_num);
			context.Registers().eax = B_ERROR;
			return B_ERROR;
	}
}
