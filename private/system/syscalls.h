#ifndef PRIVATE_SYSTEM_SYSCALLS_H
#define PRIVATE_SYSTEM_SYSCALLS_H

#include "SupportDefs.h"

// Haiku private system syscalls - minimal stub
// Essential definitions for UserlandVM-HIT compilation

extern void _kern_exit_team(status_t returnValue);

#endif /* PRIVATE_SYSTEM_SYSCALLS_H */