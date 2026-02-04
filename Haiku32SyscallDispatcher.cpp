/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifdef __linux__
#include <features.h>
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <OS.h>

#include "AddressSpace.h"
#include "DebugOutput.h"
#include "Haiku32SyscallDispatcher.h"
#include "X86_32GuestContext.h"

Haiku32SyscallDispatcher::Haiku32SyscallDispatcher(AddressSpace *addressSpace)
    : fAddressSpace(addressSpace) {
  for (int i = 0; i < MAX_FDS; i++) {
    fOpenFds[i] = -1;
  }
  // Standard FDs
  fOpenFds[0] = STDIN_FILENO;
  fOpenFds[1] = STDOUT_FILENO;
  fOpenFds[2] = STDERR_FILENO;
}

Haiku32SyscallDispatcher::~Haiku32SyscallDispatcher() {}

status_t Haiku32SyscallDispatcher::Dispatch(GuestContext &generic_context) {
  // Cast to X86_32GuestContext
  X86_32GuestContext &context =
      static_cast<X86_32GuestContext &>(generic_context);
  X86_32Registers &regs = context.Registers();

  // Haiku x86-32 ABI:
  // EAX = syscall number
  // EDX = pointer to arguments on guest stack
  uint32 syscall_num = regs.eax;
  uint32 args_ptr = regs.edx;

  // Helper lambda to get argument by index from stack pointed by EDX
  auto get_arg = [&](int index) -> uint32 {
    uint32 val = 0;
    fAddressSpace->ReadMemory(args_ptr + (index * 4), &val, 4);
    return val;
  };

  status_t status = B_OK;
  uint32 result = 0;

  switch (syscall_num) {
  case SYSCALL_EXIT: {
    int32 code = (int32)get_arg(0);
    status = SyscallExit(code);
    break;
  }

  case SYSCALL_WRITE: {
    uint32 fd = get_arg(0);
    uint32 buffer_vaddr = get_arg(1);
    uint32 size = get_arg(2);
    status = SyscallWrite(
        fd, reinterpret_cast<void *>(static_cast<uintptr_t>(buffer_vaddr)),
        size, result);
    break;
  }

  case SYSCALL_READ: {
    uint32 fd = get_arg(0);
    uint32 buffer_vaddr = get_arg(1);
    uint32 size = get_arg(2);
    status = SyscallRead(
        fd, reinterpret_cast<void *>(static_cast<uintptr_t>(buffer_vaddr)),
        size, result);
    break;
  }

  case SYSCALL_OPEN: {
    uint32 path_ptr = get_arg(1);
    uint32 flags = get_arg(2);
    uint32 mode = get_arg(3);
    status = SyscallOpen(
        reinterpret_cast<const char *>(static_cast<uintptr_t>(path_ptr)), flags,
        mode, result);
    break;
  }

  case SYSCALL_CLOSE: {
    uint32 fd = get_arg(0);
    status = SyscallClose(fd, result);
    break;
  }

  case SYSCALL_CREATE_PORT: {
    int32 queue_length = (int32)get_arg(0);
    // uint32 name_ptr = get_arg(1);
    result = 1000 + queue_length; // Dummy port ID
    status = B_OK;
    break;
  }

  case SYSCALL_WRITE_PORT_ETC: {
    result = 0;
    status = B_OK;
    break;
  }

  case SYSCALL_READ_PORT_ETC: {
    result = 0;
    status = B_OK;
    break;
  }

  default:
    DebugPrintf("UNIMPLEMENTED Haiku Syscall %u\n", syscall_num);
    status = B_BAD_VALUE;
    break;
  }

  if (status == B_OK) {
    regs.eax = result;
  } else {
    regs.eax = (uint32)(-status);
  }

  if (syscall_num == SYSCALL_EXIT) {
    context.SetExit(true);
  }

  return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallExit(int32 code) {
  (void)code;
  return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallWrite(uint32 fd, const void *buffer,
                                                uint32 size, uint32 &result) {
  if (!buffer || size == 0) {
    result = 0;
    return B_OK;
  }

  if (!fAddressSpace)
    return B_BAD_VALUE;

  uint32_t guest_vaddr = (uint32_t)(uintptr_t)buffer;
  std::vector<char> temp_buffer(size);

  status_t err =
      fAddressSpace->ReadMemory(guest_vaddr, temp_buffer.data(), size);
  if (err != B_OK)
    return err;

  ssize_t written = 0;
  // Map guest FD to host FD
  int host_fd = (fd < MAX_FDS) ? fOpenFds[fd] : -1;
  if (host_fd == -1 && fd <= 2)
    host_fd = fd; // Fallback for stdio

  if (host_fd != -1) {
    written = write(host_fd, temp_buffer.data(), size);
    if (host_fd == STDOUT_FILENO || host_fd == STDERR_FILENO) {
      tcdrain(host_fd); // More robust than fsync for terminals
    }
  } else {
    written = size; // Stub success for unknown FDs
  }

  if (written < 0) {
    result = (uint32)-1;
    return errno;
  }

  result = (uint32)written;
  return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallRead(uint32 fd, void *buffer,
                                               uint32 size, uint32 &result) {
  if (!buffer || size == 0 || !fAddressSpace) {
    result = 0;
    return B_OK;
  }

  uint32_t guest_vaddr = (uint32_t)(uintptr_t)buffer;
  std::vector<char> temp_buffer(size);

  int host_fd = (fd < MAX_FDS) ? fOpenFds[fd] : -1;
  if (host_fd == -1 && fd == 0)
    host_fd = STDIN_FILENO;

  if (host_fd == -1) {
    result = (uint32)-1;
    return B_FILE_ERROR;
  }

  ssize_t bytes_read = read(host_fd, temp_buffer.data(), size);
  if (bytes_read < 0) {
    result = (uint32)-1;
    return errno;
  }

  status_t status =
      fAddressSpace->WriteMemory(guest_vaddr, temp_buffer.data(), bytes_read);
  if (status != B_OK)
    return status;

  result = (uint32)bytes_read;
  return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallOpen(const char *path, int32 flags,
                                               int32 mode, uint32 &result) {
  if (!path || !fAddressSpace)
    return B_BAD_VALUE;

  uint32_t guest_vaddr = (uint32_t)(uintptr_t)path;
  char path_buffer[512];
  status_t read_status =
      fAddressSpace->ReadString(guest_vaddr, path_buffer, sizeof(path_buffer));
  if (read_status != B_OK)
    return read_status;

  // TODO: Map flags properly between Haiku and Linux
  int host_fd = open(path_buffer, flags, mode);
  if (host_fd < 0) {
    result = (uint32)-1;
    return errno;
  }

  // Assign to free guest FD
  for (int i = 3; i < MAX_FDS; i++) {
    if (fOpenFds[i] == -1) {
      fOpenFds[i] = host_fd;
      result = i;
      return B_OK;
    }
  }

  close(host_fd);
  return B_NO_MORE_FDS;
}

status_t Haiku32SyscallDispatcher::SyscallClose(uint32 fd, uint32 &result) {
  if (fd >= MAX_FDS || fOpenFds[fd] == -1) {
    result = (uint32)-1;
    return B_FILE_ERROR;
  }

  int host_fd = fOpenFds[fd];
  fOpenFds[fd] = -1;

  if (host_fd > 2) {
    close(host_fd);
  }

  result = 0;
  return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallSeek(uint32 fd, uint32 offset,
                                               int32 whence, uint32 &result) {
  (void)fd;
  (void)offset;
  (void)whence;
  (void)result;
  return B_ERROR;
}

status_t Haiku32SyscallDispatcher::SyscallGetCwd(char *buffer, uint32 size,
                                                 uint32 &result) {
  (void)buffer;
  (void)size;
  (void)result;
  return B_ERROR;
}

status_t Haiku32SyscallDispatcher::SyscallChdir(const char *path,
                                                uint32 &result) {
  (void)path;
  (void)result;
  return B_ERROR;
}

status_t Haiku32SyscallDispatcher::SyscallBrk(uint32 addr, uint32 &result) {
  (void)addr;
  (void)result;
  return B_ERROR;
}
