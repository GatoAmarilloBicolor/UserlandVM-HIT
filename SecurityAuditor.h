#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>

// Security Audit and Hardening System
// Provides comprehensive security analysis and protection for UserlandVM

struct SecurityViolation {
    enum Severity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    Severity severity;
    std::string description;
    std::string component;
    uint64_t timestamp;
    uint32_t address;
    std::string stack_trace;
    
    SecurityViolation(Severity sev, const std::string& desc, const std::string& comp, uint32_t addr = 0)
        : severity(sev), description(desc), component(comp), timestamp(0), address(addr) {}
};

struct SecurityConfig {
    bool enable_sandboxing;
    bool validate_syscalls;
    bool limit_file_access;
    bool enable_audit_logging;
    bool enable_memory_protection;
    bool enable_stack_protection;
    bool enable_instruction_validation;
    bool enable_address_space_layout_randomization;
    size_t max_memory_per_process;
    uint32_t max_instructions_per_execution;
    uint32_t execution_timeout_ms;
    std::vector<std::string> allowed_file_paths;
    std::vector<std::string> blocked_syscalls;
    std::vector<std::string> allowed_network_ports;
};

class SecurityAuditor {
private:
    std::vector<SecurityViolation> fViolations;
    std::unordered_map<std::string, uint64_t> fViolationCounts;
    std::unordered_set<uint32_t> fWatchAddresses;
    SecurityConfig fConfig;
    std::mutex fAuditMutex;
    
    // Security analysis data
    std::unordered_map<uint32_t, uint8_t> fExecutablePages;
    std::vector<uint32_t> fStackCanary;
    std::vector<uint32_t> fHeapCanaries;
    
public:
    SecurityAuditor();
    ~SecurityAuditor();
    
    // Configuration
    void SetConfiguration(const SecurityConfig& config);
    SecurityConfig GetConfiguration() const { return fConfig; }
    
    // Core security methods
    bool ValidateMemoryAccess(uint32_t address, size_t size, bool is_write);
    bool ValidateSyscall(uint32_t syscall_num, const std::vector<uint32_t>& args);
    bool ValidateInstruction(uint32_t opcode, const uint8_t* instruction_data);
    bool ValidateFileAccess(const std::string& path, int mode);
    
    // Auditing and reporting
    void LogViolation(const SecurityViolation& violation);
    void AnalyzeExecutionPattern();
    void GenerateSecurityReport();
    std::vector<SecurityViolation> GetViolations() const { return fViolations; }
    
    // Hardening methods
    void EnableSandbox();
    void SetupMemoryProtection();
    void SetupStackProtection();
    void SetupHeapProtection();
    void RandomizeAddressSpace();
    
    // Specific security checks
    bool DetectBufferOverflow(uint32_t address, size_t access_size);
    bool DetectHeapCorruption();
    bool DetectStackSmashing();
    bool DetectReturnOrientedProgramming();
    bool DetectInjectionAttempts(const uint8_t* data, size_t size);
    bool DetectPrivilegeEscalation(uint32_t syscall_num);
    
    // Memory safety
    void* SecureAllocate(size_t size, const char* file = nullptr, int line = 0);
    void SecureDeallocate(void* ptr);
    void* SecureReallocate(void* ptr, size_t new_size);
    
    // Runtime protection
    void InstallStackCanaries();
    void InstallHeapCanaries();
    void ValidateControlFlow();
    
    // Audit logging
    void EnableAuditLogging(bool enable);
    void ExportAuditLog(const std::string& filename);
    void PrintSecuritySummary();
    
private:
    // Internal security helpers
    bool IsValidAddress(uint32_t address);
    bool IsAllowedFileAccess(const std::string& path, int mode);
    bool IsAllowedSyscall(uint32_t syscall_num);
    void CheckCanaryIntegrity(uint32_t canary, uint32_t expected);
    void RandomizeMemoryRegion(uint32_t start_addr, size_t size);
    
    // Threat detection
    bool AnalyzeInstructionSequence(const std::vector<uint32_t>& instructions);
    bool DetectSuspiciousPattern(const uint8_t* data, size_t size);
    void CreateAlert(const SecurityViolation& violation);
    
    // Stack protection
    void SetupStackCanaries();
    bool ValidateStackFrame(uint32_t frame_ptr);
    
    // Heap protection
    void SetupHeapCanaries();
    bool ValidateHeapBlock(void* ptr, size_t size);
    
    // Instruction validation
    bool ValidateInstructionOpcode(uint32_t opcode);
    bool ValidateInstructionOperands(uint32_t opcode, const uint8_t* data);
    
    // Thread safety
    void LockAudit() { fAuditMutex.lock(); }
    void UnlockAudit() { fAuditMutex.unlock(); }
};

// Security Hardening Implementation
class SecurityHardener {
private:
    SecurityAuditor* fAuditor;
    
public:
    SecurityHardener(SecurityAuditor* auditor);
    ~SecurityHardener();
    
    // Hardening methods
    void HardenInterpreter();
    void HardenSyscallDispatcher();
    void HardenMemoryManager();
    void HardenExecutionEngine();
    
    // Specific hardening
    void AddBoundsChecking();
    void AddInputValidation();
    void AddOutputSanitization();
    void AddControlFlowIntegrity();
    
    // Compile-time hardening
    void EnableCompilerSecurityFlags();
    void EnableLinkerSecurityFlags();
    
    // Runtime hardening
    void EnableRuntimeProtections();
    void InstallSignalHandlers();
    void SetupErrorHandling();
};

// Vulnerability Scanner
class VulnerabilityScanner {
public:
    struct Vulnerability {
        std::string type;
        std::string description;
        std::string severity;
        std::string component;
        std::vector<std::string> cwe_ids;
        std::string recommendation;
    };
    
private:
    std::vector<Vulnerability> fVulnerabilities;
    
public:
    VulnerabilityScanner();
    ~VulnerabilityScanner();
    
    // Scanning methods
    void ScanCode(const std::string& file_path);
    void ScanBinary(const std::string& binary_path);
    void ScanConfiguration();
    void ScanDependencies();
    
    // Specific vulnerability checks
    void ScanForBufferOverflows();
    void ScanForInjectionVulnerabilities();
    void ScanForRaceConditions();
    void ScanForCryptographicWeaknesses();
    void ScanForInformationDisclosure();
    void ScanForPrivilegeEscalation();
    
    // Reporting
    std::vector<Vulnerability> GetVulnerabilities() const { return fVulnerabilities; }
    void GenerateVulnerabilityReport();
    void ExportVulnerabilityReport(const std::string& filename);
    void PrintVulnerabilitySummary();
};

// Intrusion Detection System
class IntrusionDetector {
private:
    std::vector<std::string> fSuspiciousPatterns;
    std::unordered_map<std::string, uint32_t> fAttemptCounts;
    std::chrono::steady_clock::time_point fLastReset;
    
    struct IntrusionAttempt {
        std::string pattern;
        std::string source;
        uint64_t timestamp;
        bool blocked;
    };
    
    std::vector<IntrusionAttempt> fAttempts;
    std::mutex fDetectionMutex;
    
public:
    IntrusionDetector();
    ~IntrusionDetector();
    
    // Detection methods
    bool DetectSuspiciousActivity(const std::string& activity);
    bool DetectInjectionAttempt(const uint8_t* data, size_t size);
    bool DetectBruteForceAttempt(const std::string& service, const std::string& source);
    bool DetectPrivilegeEscalationAttempt(const std::string& syscall);
    
    // Pattern management
    void AddSuspiciousPattern(const std::string& pattern);
    void UpdateAttemptCounts(const std::string& pattern, const std::string& source);
    
    // Blocking and response
    void BlockSource(const std::string& source);
    void ResetAfterTimeout();
    
    // Reporting
    void GenerateIntrusionReport();
    void ExportIntrusionLog(const std::string& filename);
    void PrintIntrusionSummary();
    
private:
    void LockDetection() { fDetectionMutex.lock(); }
    void UnlockDetection() { fDetectionMutex.unlock(); }
};

// Security Macros
#define SECURITY_VALIDATE_ACCESS(addr, size, is_write) \
    do { \
        static SecurityAuditor* _sec_auditor = nullptr; \
        if (!_sec_auditor) _sec_auditor = new SecurityAuditor(); \
        if (!_sec_auditor->ValidateMemoryAccess(addr, size, is_write)) { \
            _sec_auditor->LogViolation(SecurityViolation::CRITICAL, \
                "Invalid memory access detected", \
                "MemoryValidator", addr); \
        } \
    } while(0)

#define SECURITY_VALIDATE_SYSCALL(num, args) \
    do { \
        static SecurityAuditor* _sec_auditor = nullptr; \
        if (!_sec_auditor) _sec_auditor = new SecurityAuditor(); \
        if (!_sec_auditor->ValidateSyscall(num, args)) { \
            _sec_auditor->LogViolation(SecurityViolation::WARNING, \
                "Syscall validation failed", \
                "SyscallValidator", num); \
        } \
    } while(0)

#define SECURITY_VALIDATE_INSTRUCTION(opcode, data) \
    do { \
        static SecurityAuditor* _sec_auditor = nullptr; \
        if (!_sec_auditor) _sec_auditor = new SecurityAuditor(); \
        if (!_sec_auditor->ValidateInstruction(opcode, data)) { \
            _sec_auditor->LogViolation(SecurityViolation::ERROR, \
                "Invalid instruction detected", \
                "InstructionValidator", opcode); \
        } \
    } while(0)

#define SECURE_ALLOCATE(size) \
    SecurityAuditor::_SecureAllocate(size, __FILE__, __LINE__)

#define SECURE_FREE(ptr) \
    SecurityAuditor::_SecureDeallocate(ptr)

#define SECURE_REALLOC(ptr, size) \
    SecurityAuditor::_SecureReallocate(ptr, size)

// Common Vulnerability Patterns
class VulnerabilityPatterns {
public:
    static const std::string BUFFER_OVERFLOW_PATTERNS[];
    static const std::string INJECTION_PATTERNS[];
    static const std::string RACE_CONDITION_PATTERNS[];
    static const std::string CRYPTO_WEAKNESS_PATTERNS[];
    
    static bool MatchesPattern(const std::string& data, const std::string* patterns, size_t count);
    static bool DetectBufferOverflow(const uint8_t* data, size_t size);
    static bool DetectSqlInjection(const std::string& input);
    static bool DetectCommandInjection(const std::string& input);
    static bool DetectCrossSiteScripting(const std::string& input);
};

// Security Constants
namespace SecurityConstants {
    constexpr uint32_t STACK_CANARY = 0xDEADBEEF;
    constexpr uint32_t HEAP_CANARY = 0xFEEDFACE;
    constexpr size_t CANARY_SIZE = sizeof(uint32_t);
    constexpr uint32_t MEMORY_PAGE_SIZE = 4096;
    constexpr uint32_t EXECUTABLE_MEMORY_SIZE = 0x10000000;  // 256MB
    
    // Security violation types
    constexpr const char* VIOLATION_TYPES[] = {
        "Buffer Overflow",
        "Heap Corruption",
        "Stack Smashing",
        "Injection Attack",
        "Privilege Escalation",
        "Information Disclosure",
        "Invalid Memory Access",
        "Invalid Syscall",
        "Suspicious Instruction",
        "Control Flow Integrity"
    };
    
    // Severity levels
    enum Severity {
        INFO = 0,
        WARNING = 1,
        ERROR = 2,
        CRITICAL = 3
    };
    
    // Security policies
    struct SecurityPolicy {
        bool allow_arbitrary_code_execution;
        bool allow_dynamic_loading;
        bool allow_network_access;
        bool allow_file_system_access;
        bool allow_syscalls;
        bool allow_memory_modification;
        bool allow_debugging;
    };
    
    // Default restrictive policy
    constexpr SecurityPolicy DEFAULT_POLICY = {
        false,  // No arbitrary code execution
        false,  // No dynamic loading
        false,  // No network access
        true,   // Limited file system access
        true,   // Limited syscalls
        false,  // No memory modification
        false    // No debugging
    };
    
    // Memory protection flags
    constexpr uint32_t MEMORY_READ_ONLY = 0x1;
    constexpr uint32_t MEMORY_WRITE_ONLY = 0x2;
    constexpr uint32_t MEMORY_READ_WRITE = 0x3;
    constexpr uint32_t MEMORY_NO_EXECUTE = 0x4;
    
    // Syscall restrictions
    const std::vector<uint32_t> DANGEROUS_SYSCALLS = {
        0x05, // clone
        0x3B, // execve
        0x0A, // mprotect
        0x16, // ptrace
        0x57, // fork
        0x27  // create_module
    };
    
    // File access restrictions
    const std::vector<std::string> SENSITIVE_PATHS = {
        "/etc/", "/proc/", "/sys/",
        "/root/", "/home/", "/var/",
        "/usr/bin/", "/usr/sbin/",
        "/boot/", "/dev/"
    };
}

// Security Utilities
namespace SecurityUtils {
    // String sanitization
    std::string SanitizeInput(const std::string& input);
    std::string SanitizeFilename(const std::string& filename);
    std::string SanitizePath(const std::string& path);
    
    // Validation utilities
    bool IsValidPointer(uint32_t ptr);
    bool IsValidAddress(uint32_t addr, size_t size);
    bool IsValidString(const char* str, size_t max_length);
    
    // Encoding/decoding
    std::string Base64Encode(const std::string& data);
    std::string Base64Decode(const std::string& encoded_data);
    std::string HexEncode(const uint8_t* data, size_t size);
    std::vector<uint8_t> HexDecode(const std::string& hex_string);
    
    // Hashing
    std::string ComputeSHA256(const std::string& data);
    std::string ComputeSHA1(const std::string& data);
    std::string ComputeMD5(const std::string& data);
    
    // Random generation
    uint32_t GenerateSecureRandom();
    std::string GenerateSecureToken(size_t length);
    
    // Time-based security
    uint64_t GetSecureTimestamp();
    bool IsTimestampValid(uint64_t timestamp, uint64_t max_age_ms);
    
    // Comparison utilities
    bool ConstantTimeCompare(const void* a, const void* b, size_t size);
    bool SecureStringEquals(const std::string& a, const std::string& b);
}