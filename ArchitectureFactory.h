/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * ArchitectureFactory.h - Factory for creating architecture-specific components
 */

#ifndef ARCHITECTURE_FACTORY_H
#define ARCHITECTURE_FACTORY_H

#include <memory>
#include <string>

// Forward declarations
class AddressSpace;
class ExecutionEngine;
class GuestContext;
class SyscallDispatcher;

enum class TargetArchitecture {
    HAIKU_X86_32,
    HAIKU_X86_64,
    HAIKU_RISCV64,
    LINUX_X86_64,
    AUTO_DETECT
};

class ArchitectureFactory {
public:
    // Main factory methods
    static std::unique_ptr<AddressSpace> CreateAddressSpace(TargetArchitecture arch);
    static std::unique_ptr<ExecutionEngine> CreateExecutionEngine(TargetArchitecture arch, AddressSpace* space);
    static std::unique_ptr<GuestContext> CreateGuestContext(TargetArchitecture arch);
    static std::unique_ptr<SyscallDispatcher> CreateSyscallDispatcher(TargetArchitecture arch, AddressSpace* space);

    // Architecture detection
    static TargetArchitecture DetectArchitecture(const std::string& binaryPath);
    static TargetArchitecture DetectFromMagic(uint32_t magic);
    static std::string GetArchitectureName(TargetArchitecture arch);

    // Platform-specific factory methods
    static std::unique_ptr<AddressSpace> CreateHaikuX86_32AddressSpace();
    static std::unique_ptr<AddressSpace> CreateHaikuX86_64AddressSpace();
    static std::unique_ptr<AddressSpace> CreateHaikuRISCV64AddressSpace();
    static std::unique_ptr<AddressSpace> CreateLinuxX86_64AddressSpace();

    static std::unique_ptr<ExecutionEngine> CreateHaikuX86_32Engine(AddressSpace* space);
    static std::unique_ptr<ExecutionEngine> CreateHaikuX86_64Engine(AddressSpace* space);
    static std::unique_ptr<ExecutionEngine> CreateHaikuRISCV64Engine(AddressSpace* space);
    static std::unique_ptr<ExecutionEngine> CreateLinuxX86_64Engine(AddressSpace* space);

    static std::unique_ptr<SyscallDispatcher> CreateHaikuX86_32SyscallDispatcher(AddressSpace* space);
    static std::unique_ptr<SyscallDispatcher> CreateHaikuX86_64SyscallDispatcher(AddressSpace* space);
    static std::unique_ptr<SyscallDispatcher> CreateHaikuRISCV64SyscallDispatcher(AddressSpace* space);
    static std::unique_ptr<SyscallDispatcher> CreateLinuxX86_64SyscallDispatcher(AddressSpace* space);

private:
    ArchitectureFactory() = default;
    ~ArchitectureFactory() = default;
    ArchitectureFactory(const ArchitectureFactory&) = delete;
    ArchitectureFactory& operator=(const ArchitectureFactory&) = delete;
};

#endif // ARCHITECTURE_FACTORY_H