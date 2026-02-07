#include "PlatformTypes.h"
#include "Syscalls.h"
#include <private/system/syscalls.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>

thread_id vm_spawn_thread(struct thread_creation_attributes* attributes);
void vm_exit_thread(status_t returnValue);
thread_id vm_fork();
thread_id vm_load_image(const char* const* flatArgs, size_t flatArgsSize, int32 argCount, int32 envCount, int32 priority, uint32 flags, port_id errorPort, uint32 errorToken);
status_t vm_exec(const char *path, const char* const* flatArgs, size_t flatArgsSize, int32 argCount, int32 envCount, mode_t umask);

status_t vm_sigaction(int sig, const struct sigaction *action, struct sigaction *oldAction);

#define _kern_spawn_thread vm_spawn_thread
#define _kern_exit_thread vm_exit_thread
#define _kern_fork vm_fork
#define _kern_load_image vm_load_image
#define _kern_exec vm_exec


// Minimal syscall dispatcher for stable baseline
void DispatchSyscall(uint32 op, uint64 *args, uint64 *_returnValue)
{
	// Stub implementation - just return 0 for now
	*_returnValue = 0;
}
