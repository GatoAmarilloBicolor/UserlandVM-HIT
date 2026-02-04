/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CommpageManager.h"
#include <OS.h>
#include <SupportDefs.h>
#include <commpage_defs.h>
#include <stdio.h>
#include <string.h>

const uint8_t CommpageManager::sX86SyscallStub[] = {
    0xCD, 0x63, // int $0x63
    0xC3        // ret
};

extern area_id vm32_create_area(const char *name, void **address,
                                uint32 addressSpec, size_t size, uint32 lock,
                                uint32 protection);

status_t CommpageManager::Setupx86Commpage(AddressSpace &addressSpace,
                                           uint32_t &guestAddress) {
  printf(
      "[Commpage] Setting up Haiku x86-32 commpage using vm32_create_area\n");

  // Create a real Haiku area for the commpage in 32-bit space
  void *addr = NULL;
  area_id area =
      vm32_create_area("commpage", &addr, B_ANY_ADDRESS, COMMPAGE_SIZE,
                       B_NO_LOCK, B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA);

  if (area < B_OK) {
    fprintf(stderr, "[Commpage] ERROR: Failed to create commpage area: %s\n",
            strerror(area));
    return area;
  }

  uint32_t vaddr = (uint32_t)(unsigned long long)addr;
  guestAddress = vaddr;
  printf("[Commpage] Area created at 0x%08x\n", vaddr);

  // 1. Fill basic headers
  uint32_t magic = COMMPAGE_SIGNATURE;
  uint32_t version = COMMPAGE_VERSION;

  addressSpace.Write(vaddr + (COMMPAGE_ENTRY_MAGIC * 4), magic);
  addressSpace.Write(vaddr + (COMMPAGE_ENTRY_VERSION * 4), version);

  // 2. Prepare syscall entry
  // We'll place the actual code after the table (64 entries * 4 bytes = 256
  // bytes)
  uint32_t codeOffset = 0x100;
  uint32_t syscallCodeVaddr = vaddr + codeOffset;

  addressSpace.WriteMemory(syscallCodeVaddr, sX86SyscallStub,
                           sizeof(sX86SyscallStub));

  // Point the table entry to the code
  // NOTE: In Haiku x86, the table contains OFFSETS from the commpage base, or
  // absolute addresses? Based on libroot source: addl 4 *
  // COMMPAGE_ENTRY_X86_SYSCALL(%edx), %edx This means the table entry at index
  // COMMPAGE_ENTRY_X86_SYSCALL contains an offset? Wait, libroot says: "addl 4
  // * COMMPAGE_ENTRY_X86_SYSCALL(%edx), %edx" -> No, it's actually: "movl
  // __gCommPageAddress, %edx; addl 4 * COMMPAGE_ENTRY_X86_SYSCALL(%edx), %edx"
  // This is slightly confusing assembly. Usually it means EDX += table[entry].

  addressSpace.Write(vaddr + (COMMPAGE_ENTRY_X86_SYSCALL * 4), codeOffset);

  printf("[Commpage] Registered syscall stub at 0x%08x (offset 0x%x in "
         "commpage)\n",
         syscallCodeVaddr, codeOffset);

  return B_OK;
}
