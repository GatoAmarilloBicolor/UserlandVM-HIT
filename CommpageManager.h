/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "AddressSpace.h"
#include <SupportDefs.h>
#include <commpage_defs.h>

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
