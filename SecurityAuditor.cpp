#include "SecurityAuditor.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>

// Implementation of SecurityAuditor

SecurityAuditor::SecurityAuditor()
    : fConfig({
        true,  // enable_sandboxing
        true,  // validate_syscalls
        true,  // limit_file_access
        true,  // enable_audit_logging
        true,  // enable_memory_protection
        true,  // enable_stack_protection
        true,  // enable_instruction_validation
        false, // enable_address_space_layout_randomization
        1024 * 1024 * 1024, // max_memory_per_process (1GB)
        1000000000, // max_instructions_per_execution (1B)
        30000, // execution_timeout_ms (30s)
        {"/tmp"}, // allowed_file_paths
        {0x0A, 0x5C, 0x5E, 0x3F, 0x3B, 0x3C}, // blocked_syscalls (clone, execve, mprotect, ptrace)
        {}       // allowed_network_ports
    }),
    fExecutablePages(0x1000), fStackCanary(256), fHeapCanaries(256) {
    // Initialize executable memory tracking
}

SecurityAuditor::~SecurityAuditor() {
    GenerateSecurityReport();
}

void SecurityAuditor::SetConfiguration(const SecurityConfig& config) {
    std::lock_guard<std::mutex> lock(fAuditMutex);
    fConfig = config;
    
    if (config.enable_sandboxing) EnableSandbox();
    if (config.enable_memory_protection) SetupMemoryProtection();
    if (config.enable_stack_protection) SetupStackProtection();
    if (config.enable_heap_canaries) SetupHeapCanaries();
    if (config.enable_instruction_validation) ValidateControlFlow();
}

bool SecurityAuditor::ValidateMemoryAccess(uint32_t address, size_t size, bool is_write) {
    std::lock_guard<std::mutex> lock(fAuditMutex);
    
    // Check if address is in valid range
    if (!IsValidAddress(address)) {
        LogViolation(SecurityViolation(CRITICAL, 
            "Invalid memory access address", 
            "MemoryValidator", address));
        return false;
    }
    
    // Check if access crosses page boundaries
    uint32_t end_addr = address + size;
    if ((address & 0xFFF) != (end_addr & 0xFFF)) {
        LogViolation(SecurityViolation(WARNING, 
            "Page crossing memory access detected", 
            "MemoryValidator", address));
    }
    
    // Check executable memory permissions
    uint32_t page_start = address & ~(SecurityConstants::MEMORY_PAGE_SIZE - 1);
    auto page_it = fExecutablePages.find(page_start);
    
    if (is_write && page_it != fExecutablePages.end() && 
        (page_it->second & SecurityConstants::MEMORY_READ_ONLY)) {
        LogViolation(SecurityViolation(ERROR, 
            "Write access to read-only memory page", 
            "MemoryValidator", address));
        return false;
    }
    
    return true;
}

bool SecurityAuditor::ValidateSyscall(uint32_t syscall_num, const std::vector<uint32_t>& args) {
    std::lock_guard<std::mutex> lock(fAuditMutex);
    
    // Check if syscall is blocked
    if (std::find(SecurityConstants::DANGEROUS_SYSCALLS.begin(), 
                   SecurityConstants::DANGEROUS_SYSCALLS.end(), syscall_num) != 
                   SecurityConstants::DANGEROUS_SYSCALLS.end()) {
        LogViolation(SecurityViolation(CRITICAL, 
            "Dangerous syscall attempted", 
            "SyscallValidator", syscall_num));
        return false;
    }
    
    // Validate syscall arguments
    for (size_t i = 1; i < args.size(); i++) {
        uint32_t arg = args[i];
        if (arg > 0xC0000000) {  // Check for null pointer dereference in args
            LogViolation(SecurityViolation(WARNING, 
                "Potential null pointer dereference in syscall argument", 
                "SyscallValidator", syscall_num));
        }
    }
    
    return true;
}

bool SecurityAuditor::ValidateInstruction(uint32_t opcode, const uint8_t* instruction_data) {
    std::lock_guard<std::mutex> lock(fAuditMutex);
    
    // Validate opcode
    if (!ValidateInstructionOpcode(opcode)) {
        LogViolation(SecurityViolation(ERROR, 
            "Invalid opcode encountered", 
            "InstructionValidator", opcode));
        return false;
    }
    
    // Check for suspicious instruction patterns
    if (DetectSuspiciousPattern(instruction_data, 15)) {
        LogViolation(SecurityViolation(WARNING, 
            "Suspicious instruction pattern detected", 
            "PatternDetector", opcode));
    }
    
    return true;
}

bool SecurityAuditor::ValidateFileAccess(const std::string& path, int mode) {
    std::lock_guard<std::mutex> lock(fAuditMutex);
    
    // Check if path is allowed
    if (!IsAllowedFileAccess(path, mode)) {
        LogViolation(SecurityViolation(ERROR, 
            "Access to sensitive file path", 
            "FileAccessValidator", 0));
        return false;
    }
    
    // Check for directory traversal attempts
    if (path.find("../") != std::string::npos) {
        LogViolation(SecurityViolation(WARNING, 
            "Potential directory traversal attempt", 
            "FileAccessValidator", 0));
        return false;
    }
    
    return true;
}

void SecurityAuditor::LogViolation(const SecurityViolation& violation) {
    fViolations.push_back(violation);
    
    // Update violation counts
    std::string key = violation.component + ":" + violation.description;
    fViolationCounts[key]++;
    
    // Print to console if critical
    if (violation.severity == SecurityViolation::CRITICAL) {
        std::cerr << "🚨 SECURITY ALERT: " << violation.description 
                   << " in " << violation.component << std::endl;
    }
    
    // Write to audit log if enabled
    if (fConfig.enable_audit_logging) {
        std::ofstream audit_file("security_audit.log", std::ios::app);
        if (audit_file.is_open()) {
            audit_file << "[" << violation.timestamp << "] "
                      << violation.severity << ": "
                      << violation.description << " in "
                      << violation.component << std::endl;
        }
    }
}

void SecurityAuditor::AnalyzeExecutionPattern() {
    // Analyze recent violation patterns
    if (fViolations.empty()) return;
    
    std::cout << "=== EXECUTION PATTERN ANALYSIS ===" << std::endl;
    
    // Group violations by component
    std::unordered_map<std::string, std::vector<const SecurityViolation*>> component_violations;
    for (const auto& violation : fViolations) {
        component_violations[violation.component].push_back(&violation);
    }
    
    for (const auto& pair : component_violations) {
        std::cout << "Component: " << pair.first << std::endl;
        std::cout << "Violations: " << pair.second.size() << std::endl;
        
        for (const auto* v : pair.second) {
            std::cout << "  - " << v->description 
                     << " (Severity: " << v->severity << ")" << std::endl;
        }
        std::cout << std::endl;
    }
    
    // Analyze trends
    std::cout << "=== VIOLATION TRENDS ===" << std::endl;
    std::cout << "Total violations: " << fViolations.size() << std::endl;
    
    for (const auto& pair : fViolationCounts) {
        std::cout << pair.first << ": " << pair.second << " occurrences" << std::endl;
    }
    std::cout << std::endl;
}

void SecurityAuditor::GenerateSecurityReport() {
    std::cout << "=== SECURITY AUDIT REPORT ===" << std::endl;
    std::cout << "Generated by UserlandVM Security Auditor" << std::endl;
    std::cout << "Timestamp: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << std::endl;
    std::endl;
    
    // Configuration summary
    std::cout << "Security Configuration:" << std::endl;
    std::cout << "  Sandboxing: " << (fConfig.enable_sandboxing ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  Syscall Validation: " << (fConfig.validate_syscalls ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  Memory Protection: " << (fConfig.enable_memory_protection ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  Stack Protection: " << (fConfig.enable_stack_protection ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  Heap Canaries: " << (fConfig.enable_heap_canaries ? "ENABLED" : "DISABLED") << std::endl;
    std::endl;
    
    // Violation summary
    std::cout << "Security Violations:" << std::endl;
    std::cout << "  Total: " << fViolations.size() << std::endl;
    std::cout << "  Critical: " << std::count_if(fViolations.begin(), fViolations.end(),
                                         [](const SecurityViolation& v) { return v.severity == SecurityViolation::CRITICAL; }) << std::endl;
    std::cout << "  Error: " << std::count_if(fViolations.begin(), fViolations.end(),
                                       [](const SecurityViolation& v) { return v.severity == SecurityViolation::ERROR; }) << std::endl;
    std::cout << "  Warning: " << std::count_if(fViolations.begin(), fViolations.end(),
                                         [](const SecurityViolation& v) { return v.severity == SecurityViolation::WARNING; }) << std::endl;
    std::endl;
    
    // Recommendations
    std::cout << "Security Recommendations:" << std::endl;
    
    std::cout << "1. Enable comprehensive security monitoring" << std::endl;
    std::cout << "2. Implement proper input validation" << std::endl;
    std::cout << "3. Use memory-safe programming practices" << std::endl;
    std::cout << "4. Regular security audits and penetration testing" << std::endl;
    std::cout << "5. Keep all components updated and patched" << std::endl;
    std::cout << std::endl;
    
    ExportAuditLog("security_report.txt");
}

bool SecurityAuditor::DetectBufferOverflow(uint32_t address, size_t access_size) {
    // Simple buffer overflow detection
    if (access_size > 1024) {  // Large access pattern
        LogViolation(SecurityViolation(WARNING, 
            "Large buffer access detected", 
            "OverflowDetector", address));
    }
    
    return false;  // No overflow detected (simple implementation)
}

bool SecurityAuditor::DetectHeapCorruption() {
    // Check heap consistency
    for (size_t i = 0; i < fHeapCanaries.size(); i++) {
        CheckCanaryIntegrity(fHeapCanaries[i], SecurityConstants::HEAP_CANARY);
    }
    
    return false;
}

bool SecurityAuditor::DetectStackSmashing() {
    // Check stack canaries
    for (size_t i = 0; i < fStackCanary.size(); i++) {
        CheckCanaryIntegrity(fStackCanaries[i], SecurityConstants::STACK_CANARY);
    }
    
    return false;
}

bool SecurityAuditor::DetectReturnOrientedProgramming() {
    // Analyze call patterns for ROP gadgets
    // This is a simplified implementation
    return false;  // No ROP detected (simple implementation)
}

bool SecurityAuditor::DetectInjectionAttempts(const uint8_t* data, size_t size) {
    std::string data_str(reinterpret_cast<const char*>(data), size);
    
    // Check for common injection patterns
    if (VulnerabilityPatterns::DetectSqlInjection(data_str)) {
        LogViolation(SecurityViolation(CRITICAL, 
            "SQL injection attempt detected", 
            "InjectionDetector", 0));
        return true;
    }
    
    if (VulnerabilityPatterns::DetectCommandInjection(data_str)) {
        LogViolation(SecurityViolation(CRITICAL, 
            "Command injection attempt detected", 
            "InjectionDetector", 0));
        return true;
    }
    
    if (VulnerabilityPatterns::DetectCrossSiteScripting(data_str)) {
        LogViolation(SecurityViolation(CRITICAL, 
            "XSS attempt detected", 
            "InjectionDetector", 0));
        return true;
    }
    
    return false;
}

bool SecurityAuditor::DetectPrivilegeEscalationAttempt(uint32_t syscall_num) {
    return std::find(SecurityConstants::DANGEROUS_SYSCALLS.begin(), 
                   SecurityConstants::DANGEROUS_SYSCALLS.end(), syscall_num) != 
                   SecurityConstants::DANGEROUS_SYSCALLS.end());
}

void* SecurityAuditor::SecureAllocate(size_t size, const char* file, int line) {
    std::lock_guard<std::mutex> lock(fAuditMutex);
    
    // Validate size
    if (size > fConfig.max_memory_per_process) {
        LogViolation(SecurityViolation(ERROR, 
            "Memory allocation limit exceeded", 
            "MemoryManager", 0));
        return nullptr;
    }
    
    // Allocate with canaries
    size_t total_size = size + 2 * SecurityConstants::CANARY_SIZE;
    uint8_t* ptr = new uint8_t[total_size];
    
    // Setup heap canaries
    uint32_t* canary_ptr = reinterpret_cast<uint32_t*>(ptr);
    *canary_ptr = SecurityConstants::HEAP_CANARY;
    *(canary_ptr + 1) = SecurityConstants::HEAP_CANARY;
    
    fHeapCanaries.push_back(canary_ptr);
    
    LogViolation(SecurityViolation(INFO, 
        "Secure memory allocation", 
        "MemoryManager", reinterpret_cast<uint32_t>(canary_ptr)));
    
    return ptr;
}

void SecurityAuditor::SecureDeallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(fAuditMutex);
    
    // Find and remove heap canary
    for (auto it = fHeapCanaries.begin(); it != fHeapCanaries.end(); ++it) {
        if (*it == reinterpret_cast<uint32_t*>(ptr)) {
            fHeapCanaries.erase(it);
            break;
        }
    }
    
    delete[] static_cast<uint8_t*>(ptr);
    LogViolation(SecurityViolation(INFO, 
        "Secure memory deallocation", 
        "MemoryManager", ptr));
}

void* SecurityAuditor::SecureReallocate(void* ptr, size_t new_size) {
    if (!ptr) return SecureAllocate(new_size);
    
    SecureDeallocate(ptr);
    return SecureAllocate(new_size);
}

bool SecurityAuditor::IsValidAddress(uint32_t address) {
    // Check if address is in guest memory range
    return (address >= 0x08048000 && address < 0xC0000000);
}

bool SecurityAuditor::IsAllowedFileAccess(const std::string& path, int mode) {
    // Check against sensitive paths
    for (const auto& sensitive_path : SecurityConstants::SENSITIVE_PATHS) {
        if (path.find(sensitive_path) == 0) {
            return false;
        }
    }
    
    // Check file access mode
    if ((mode & O_WRONLY) && !fConfig.limit_file_access) {
        return false;
    }
    
    return true;
}

bool SecurityAuditor::IsAllowedSyscall(uint32_t syscall_num) {
    return std::find(SecurityConstants::BLOCKED_SYSCALLS.begin(), 
                   SecurityConstants::BLOCKED_SYSCALLS.end(), syscall_num) == 
                   SecurityConstants::BLOCKED_SYSCALLS.end());
}

void SecurityAuditor::CheckCanaryIntegrity(uint32_t canary, uint32_t expected) {
    if (canary != expected) {
        LogViolation(SecurityViolation(CRITICAL, 
            "Canary corruption detected", 
            "CanaryValidator", canary));
    }
}

void SecurityAuditor::RandomizeMemoryRegion(uint32_t start_addr, size_t size) {
    if (!fConfig.enable_address_space_layout_randomization) return;
    
    std::random_device rd;
    std::mt19937_64 gen(rd);
    
    for (size_t i = 0; i < size; i += 4) {
        uint32_t random_value = gen();
        reinterpret_cast<uint32_t*>(start_addr + i)[0] = random_value;
    }
}