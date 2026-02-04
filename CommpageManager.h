/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "AddressSpace.h"
#include <SupportDefs.h>
#include <commpage_defs.h>

// x86-32 commpage entry for syscall stub
#ifndef COMMPAGE_ENTRY_X86_SYSCALL
#define COMMPAGE_ENTRY_X86_SYSCALL 0x40
#endif

class CommpageManager {
public:
  static status_t Setupx86Commpage(AddressSpace &addressSpace,
                                   uint32_t &guestAddress);

private:
  // Haiku x86-32 syscall stub code:
  // int $0x63
  // ret
  static const uint8_t sX86SyscallStub[];
};
