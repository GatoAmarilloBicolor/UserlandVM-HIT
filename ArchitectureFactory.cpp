/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * ArchitectureFactory.cpp - Implementation of architecture factory
 */

#include "ArchitectureFactory.h"
#include "AddressSpace.h"
#include "DirectAddressSpace.h"
#include "ExecutionEngine.h"
#include "GuestContext.h"
#include "SyscallDispatcher.h"

// Haiku platform includes
#include "platform/haiku/HaikuPlatform.h"

// Architecture-specific includes
#include "VirtualCpuX86Native.h"
#include "X86_32GuestContext.h"
#include "InterpreterX86_32.h"

// ELF header for architecture detection
#include <elf.h>
#include <fstream>
#include <cstring>

std::unique_ptr<AddressSpace> ArchitectureFactory::CreateAddressSpace(TargetArchitecture arch) {
    switch (arch) {
        case TargetArchitecture::HAIKU_X86_32:
            return CreateHaikuX86_32AddressSpace();
        case TargetArchitecture::HAIKU_X86_64:
            return CreateHaikuX86_64AddressSpace();
        case TargetArchitecture::HAIKU_RISCV64:
            return CreateHaikuRISCV64AddressSpace();
        case TargetArchitecture::LINUX_X86_64:
            return CreateLinuxX86_64AddressSpace();
        default:
            return nullptr;
    }
}

std::unique_ptr<ExecutionEngine> ArchitectureFactory::CreateExecutionEngine(TargetArchitecture arch, AddressSpace* space) {
    switch (arch) {
        case TargetArchitecture::HAIKU_X86_32:
            return CreateHaikuX86_32Engine(space);
        case TargetArchitecture::HAIKU_X86_64:
            return CreateHaikuX86_64Engine(space);
        case TargetArchitecture::HAIKU_RISCV64:
            return CreateHaikuRISCV64Engine(space);
        case TargetArchitecture::LINUX_X86_64:
            return CreateLinuxX86_64Engine(space);
        default:
            return nullptr;
    }
}

std::unique_ptr<GuestContext> ArchitectureFactory::CreateGuestContext(TargetArchitecture arch) {
    switch (arch) {
        case TargetArchitecture::HAIKU_X86_32:
        case TargetArchitecture::LINUX_X86_64:
            return std::make_unique<X86_32GuestContext>();
        // TODO: Implement other guest contexts
        default:
            return nullptr;
    }
}

std::unique_ptr<SyscallDispatcher> ArchitectureFactory::CreateSyscallDispatcher(TargetArchitecture arch, AddressSpace* space) {
    switch (arch) {
        case TargetArchitecture::HAIKU_X86_32:
            return CreateHaikuX86_32SyscallDispatcher(space);
        case TargetArchitecture::HAIKU_X86_64:
            return CreateHaikuX86_64SyscallDispatcher(space);
        case TargetArchitecture::HAIKU_RISCV64:
            return CreateHaikuRISCV64SyscallDispatcher(space);
        case TargetArchitecture::LINUX_X86_64:
            return CreateLinuxX86_64SyscallDispatcher(space);
        default:
            return nullptr;
    }
}

TargetArchitecture ArchitectureFactory::DetectArchitecture(const std::string& binaryPath) {
    std::ifstream file(binaryPath, std::ios::binary);
    if (!file.is_open()) {
        return TargetArchitecture::AUTO_DETECT;
    }

    // Read ELF header
    Elf32_Ehdr header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file.good()) {
        return TargetArchitecture::AUTO_DETECT;
    }

    return DetectFromMagic(header.e_machine);
}

TargetArchitecture ArchitectureFactory::DetectFromMagic(uint32_t machine) {
    switch (machine) {
        case EM_386:
            return TargetArchitecture::HAIKU_X86_32;
        case EM_X86_64:
            return TargetArchitecture::HAIKU_X86_64;
        case EM_RISCV:
            return TargetArchitecture::HAIKU_RISCV64;
        default:
            return TargetArchitecture::AUTO_DETECT;
    }
}

std::string ArchitectureFactory::GetArchitectureName(TargetArchitecture arch) {
    switch (arch) {
        case TargetArchitecture::HAIKU_X86_32:
            return "Haiku x86-32";
        case TargetArchitecture::HAIKU_X86_64:
            return "Haiku x86-64";
        case TargetArchitecture::HAIKU_RISCV64:
            return "Haiku RISC-V 64";
        case TargetArchitecture::LINUX_X86_64:
            return "Linux x86-64";
        case TargetArchitecture::AUTO_DETECT:
            return "Auto-detect";
        default:
            return "Unknown";
    }
}

// Platform-specific implementations

std::unique_ptr<AddressSpace> ArchitectureFactory::CreateHaikuX86_32AddressSpace() {
    return std::make_unique<DirectAddressSpace>();
}

std::unique_ptr<AddressSpace> ArchitectureFactory::CreateHaikuX86_64AddressSpace() {
    // TODO: Implement x86-64 Haiku address space
    return nullptr;
}

std::unique_ptr<AddressSpace> ArchitectureFactory::CreateHaikuRISCV64AddressSpace() {
    // TODO: Implement RISC-V Haiku address space
    return nullptr;
}

std::unique_ptr<AddressSpace> ArchitectureFactory::CreateLinuxX86_64AddressSpace() {
    // TODO: Implement Linux x86-64 address space
    return nullptr;
}

std::unique_ptr<ExecutionEngine> ArchitectureFactory::CreateHaikuX86_32Engine(AddressSpace* space) {
    // Try native execution first, fallback to interpreter
    auto native = std::make_unique<VirtualCpuX86Native>(space);
    if (native->IsSupported()) {
        return std::move(native);
    }
    
    // Fallback to interpreter
    return std::make_unique<InterpreterX86_32>(space);
}

std::unique_ptr<ExecutionEngine> ArchitectureFactory::CreateHaikuX86_64Engine(AddressSpace* space) {
    // TODO: Implement x86-64 execution engine
    return nullptr;
}

std::unique_ptr<ExecutionEngine> ArchitectureFactory::CreateHaikuRISCV64Engine(AddressSpace* space) {
    // TODO: Implement RISC-V execution engine  
    return nullptr;
}

std::unique_ptr<ExecutionEngine> ArchitectureFactory::CreateLinuxX86_64Engine(AddressSpace* space) {
    // TODO: Implement Linux x86-64 execution engine
    return nullptr;
}

std::unique_ptr<SyscallDispatcher> ArchitectureFactory::CreateHaikuX86_32SyscallDispatcher(AddressSpace* space) {
    return std::make_unique<Haiku32SyscallDispatcher>(space);
}

std::unique_ptr<SyscallDispatcher> ArchitectureFactory::CreateHaikuX86_64SyscallDispatcher(AddressSpace* space) {
    // TODO: Implement x86-64 syscall dispatcher
    return nullptr;
}

std::unique_ptr<SyscallDispatcher> ArchitectureFactory::CreateHaikuRISCV64SyscallDispatcher(AddressSpace* space) {
    // TODO: Implement RISC-V syscall dispatcher
    return nullptr;
}

std::unique_ptr<SyscallDispatcher> ArchitectureFactory::CreateLinuxX86_64SyscallDispatcher(AddressSpace* space) {
    // TODO: Implement Linux syscall dispatcher
    return nullptr;
}