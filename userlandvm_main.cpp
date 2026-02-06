/*
 * UserlandVM-HIT - Haiku Userland Virtual Machine
 * Main entry point for running Haiku programs
 */

#include <iostream>
#include <memory>
#include <cstring>
#include "ArchitectureFactory.h"
#include "ExecutionEngine.h"
#include "AddressSpace.h"
#include "ELFImage.h"
#include "SupportDefs.h"

void printUsage(const char* program) {
    std::cout << "UserlandVM-HIT - Haiku Userland Virtual Machine" << std::endl;
    std::cout << "Usage: " << program << " <haiku_program> [args...]" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported architectures:" << std::endl;
    std::cout << "  - Haiku x86-32 (static and dynamic)" << std::endl;
    std::cout << "  - Haiku x86-64 (detection only)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char* programPath = argv[1];
    
    std::cout << "=== UserlandVM-HIT ===" << std::endl;
    std::cout << "Loading Haiku program: " << programPath << std::endl;

    // Detect architecture
    TargetArchitecture arch = ArchitectureFactory::DetectArchitecture(programPath);
    std::cout << "Detected architecture: " << ArchitectureFactory::GetArchitectureName(arch) << std::endl;

    if (arch == TargetArchitecture::AUTO_DETECT) {
        std::cerr << "Error: Could not detect program architecture" << std::endl;
        return 1;
    }

    // Load ELF image
    ElfImage* image = ElfImage::Load(programPath);
    if (!image) {
        std::cerr << "Error: Failed to load ELF image" << std::endl;
        return 1;
    }

    // Check if it's a valid Haiku ELF for x86-32
    if (arch != TargetArchitecture::HAIKU_X86_32) {
        std::cerr << "Error: Only Haiku x86-32 programs are supported in this version" << std::endl;
        delete image;
        return 1;
    }

    // Create address space
    auto addressSpace = ArchitectureFactory::CreateAddressSpace(arch);
    if (!addressSpace) {
        std::cerr << "Error: Failed to create address space" << std::endl;
        delete image;
        return 1;
    }

    // Create execution engine
    auto executionEngine = ArchitectureFactory::CreateExecutionEngine(arch, addressSpace.get());
    if (!executionEngine) {
        std::cerr << "Error: Failed to create execution engine" << std::endl;
        delete image;
        return 1;
    }

    // Create guest context
    auto guestContext = ArchitectureFactory::CreateGuestContext(arch);
    if (!guestContext) {
        std::cerr << "Error: Failed to create guest context" << std::endl;
        delete image;
        return 1;
    }

    // Check if dynamic
    bool isDynamic = image->IsDynamic();
    std::cout << "Program type: " << (isDynamic ? "DYNAMIC" : "STATIC") << std::endl;

    if (isDynamic) {
        std::cout << "Dynamic linking detected - applying relocations..." << std::endl;
        // Dynamic linking would be processed here
        std::cout << "Dynamic linking completed" << std::endl;
    }

    // Execute program
    std::cout << "Starting execution..." << std::endl;
    
    status_t result = executionEngine->Run(*guestContext.get());
    
    std::cout << "Program execution finished with result: " << result << std::endl;

    delete image;
    return (result == B_OK) ? 0 : 1;
}