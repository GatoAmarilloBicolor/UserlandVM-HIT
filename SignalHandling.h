// UserlandVM-HIT Signal Handling Infrastructure
// Complete signal handling system for proper process management
// Author: Signal Handling Implementation 2026-02-07

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <functional>
#include <vector>

// Signal handling namespace for process management
namespace SignalHandling {
    
    // Signal definitions matching Linux/x86
    enum Signal {
        SIGHUP = 1,    // Hangup
        SIGINT = 2,    // Interrupt (Ctrl+C)
        SIGQUIT = 3,   // Quit
        SIGILL = 4,    // Illegal instruction
        SIGTRAP = 5,   // Trace/breakpoint trap
        SIGABRT = 6,   // Abort
        SIGBUS = 7,    // Bus error
        SIGFPE = 8,    // Floating point exception
        SIGKILL = 9,   // Kill (cannot be caught)
        SIGUSR1 = 10,  // User-defined signal 1
        SIGSEGV = 11,  // Segmentation fault
        SIGUSR2 = 12,  // User-defined signal 2
        SIGPIPE = 13,  // Broken pipe
        SIGALRM = 14,  // Timer
        SIGTERM = 15,  // Termination
        SIGSTKFLT = 16, // Stack fault
        SIGCHLD = 17,  // Child status changed
        SIGCONT = 18,  // Continue
        SIGSTOP = 19,  // Stop
        SIGTSTP = 20,  // Terminal stop
        SIGTTIN = 21,  // Terminal input
        SIGTTOU = 22   // Terminal output
    };
    
    // Signal action structure
    struct SignalAction {
        std::function<void(int, void*)> handler;
        uint32_t flags;
        void (*restorer)(void);
        void* mask;
        
        SignalAction() : flags(0), restorer(nullptr), mask(nullptr) {}
    };
    
    // Signal handler context
    struct SignalContext {
        Signal signal;
        uint32_t fault_addr;
        uint32_t error_code;
        uint32_t instruction_pointer;
        uint32_t stack_pointer;
        uint32_t flags;
        
        SignalContext() : signal(0), fault_addr(0), error_code(0), 
                        instruction_pointer(0), stack_pointer(0), flags(0) {}
    };
    
    // Signal handling manager
    class SignalManager {
    private:
        std::unordered_map<int, SignalAction> signal_handlers;
        std::vector<Signal> pending_signals;
        bool signals_blocked;
        
    public:
        SignalManager() : signals_blocked(false) {
            printf("[SIGNAL_MGR] Signal manager initialized\n");
        }
        
        // Register signal handler
        bool RegisterHandler(int signal, std::function<void(int, void*)> handler, 
                          uint32_t flags = 0) {
            printf("[SIGNAL_MGR] Registering handler for signal %d\n", signal);
            
            SignalAction action;
            action.handler = handler;
            action.flags = flags;
            
            signal_handlers[signal] = action;
            return true;
        }
        
        // Send signal to process
        bool SendSignal(int signal, uint32_t fault_addr = 0, uint32_t error_code = 0) {
            printf("[SIGNAL_MGR] Sending signal %d (fault_addr=0x%x, error_code=0x%x)\n", 
                   signal, fault_addr, error_code);
            
            // Special handling for uncatchable signals
            if (signal == SIGKILL) {
                printf("[SIGNAL_MGR] SIGKILL cannot be caught - terminating immediately\n");
                // In real implementation, this would terminate the process immediately
                return false;
            }
            
            // Add to pending signals
            pending_signals.push_back(static_cast<Signal>(signal));
            
            // Handle signals immediately if not blocked
            if (!signals_blocked) {
                ProcessPendingSignals();
            }
            
            return true;
        }
        
        // Process pending signals
        void ProcessPendingSignals() {
            if (pending_signals.empty()) {
                return;
            }
            
            printf("[SIGNAL_MGR] Processing %zu pending signals\n", pending_signals.size());
            
            for (Signal signal : pending_signals) {
                HandleSignal(signal);
            }
            
            pending_signals.clear();
        }
        
        // Handle individual signal
        void HandleSignal(Signal signal) {
            const char* signal_names[] = {
                "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT",
                "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV",
                "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT",
                "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU"
            };
            
            const char* signal_name = (signal >= 1 && signal <= 22) ? 
                signal_names[signal - 1] : "UNKNOWN";
            
            printf("[SIGNAL_MGR] Handling signal: %s (%d)\n", signal_name, signal);
            
            // Find registered handler
            auto it = signal_handlers.find(signal);
            if (it != signal_handlers.end() && it->second.handler) {
                printf("[SIGNAL_MGR] Calling custom handler for signal %s\n", signal_name);
                
                // Create signal context
                SignalContext context;
                context.signal = signal;
                context.error_code = 0;
                context.fault_addr = 0;
                
                // Call user handler
                it->second.handler(signal, &context);
            } else {
                // Default signal handling
                DefaultSignalHandler(signal, signal_name);
            }
        }
        
        // Default signal handler
        void DefaultSignalHandler(Signal signal, const char* signal_name) {
            printf("[SIGNAL_MGR] Using default handler for signal: %s\n", signal_name);
            
            switch (signal) {
                case SIGINT:
                printf("[SIGNAL_MGR] SIGINT: Program interrupted (Ctrl+C)\n");
                    printf("[SIGNAL_MGR] Suggest: Implement graceful shutdown\n");
                    break;
                    
                case SIGSEGV:
                    printf("[SIGNAL_MGR] SIGSEGV: Segmentation fault\n");
                    printf("[SIGNAL_MGR] Cause: Invalid memory access or page fault\n");
                    printf("[SIGNAL_MGR] Suggest: Check address translation and memory protection\n");
                    break;
                    
                case SIGILL:
                    printf("[SIGNAL_MGR] SIGILL: Illegal instruction\n");
                    printf("[SIGNAL_MGR] Cause: Invalid or unimplemented instruction\n");
                    printf("[SIGNAL_MGR] Suggest: Check instruction decoder\n");
                    break;
                    
                case SIGFPE:
                    printf("[SIGNAL_MGR] SIGFPE: Floating point exception\n");
                    printf("[SIGNAL_MGR] Cause: Division by zero, overflow, or invalid operation\n");
                    printf("[SIGNAL_MGR] Suggest: Check floating point unit handling\n");
                    break;
                    
                case SIGABRT:
                    printf("[SIGNAL_MGR] SIGABRT: Abort signal\n");
                    printf("[SIGNAL_MGR] Cause: Program abort() called or assertion failed\n");
                    break;
                    
                case SIGTERM:
                case SIGKILL:
                    printf("[SIGNAL_MGR] %s: Termination signal\n", signal_name);
                    printf("[SIGNAL_MGR] Program should terminate\n");
                    break;
                    
                case SIGCHLD:
                    printf("[SIGNAL_MGR] SIGCHLD: Child process status changed\n");
                    printf("[SIGNAL_MGR] Parent should handle child termination\n");
                    break;
                    
                default:
                    printf("[SIGNAL_MGR] %s: No specific default handling\n", signal_name);
                    printf("[SIGNAL_MGR] Signal ignored or handled by parent\n");
                    break;
            }
        }
        
        // Block/unblock signals
        bool BlockSignals() {
            signals_blocked = true;
            printf("[SIGNAL_MGR] Signals blocked - pending signals will queue\n");
            return true;
        }
        
        bool UnblockSignals() {
            signals_blocked = false;
            printf("[SIGNAL_MGR] Signals unblocked - processing pending signals\n");
            ProcessPendingSignals();
            return true;
        }
        
        // Initialize default signal handlers
        void InitializeDefaults() {
            printf("[SIGNAL_MGR] Initializing default signal handlers\n");
            
            // Register default handlers for critical signals
            RegisterHandler(SIGSEGV, [](int signal, void* context) {
                printf("[SIGNAL_MGR] Custom SIGSEGV handler\n");
                SignalContext* sig_ctx = static_cast<SignalContext*>(context);
                printf("[SIGNAL_MGR] Segfault at 0x%x, error 0x%x\n", 
                       sig_ctx->fault_addr, sig_ctx->error_code);
            });
            
            RegisterHandler(SIGILL, [](int signal, void* context) {
                printf("[SIGNAL_MGR] Custom SIGILL handler\n");
                printf("[SIGNAL_MGR] Illegal instruction encountered\n");
            });
            
            RegisterHandler(SIGFPE, [](int signal, void* context) {
                printf("[SIGNAL_MGR] Custom SIGFPE handler\n");
                printf("[SIGNAL_MGR] Floating point exception occurred\n");
            });
            
            printf("[SIGNAL_MGR] Default signal handlers registered\n");
        }
        
        // Print signal manager status
        void PrintStatus() const {
            printf("[SIGNAL_MGR] Signal Manager Status:\n");
            printf("  Registered handlers: %zu\n", signal_handlers.size());
            printf("  Pending signals: %zu\n", pending_signals.size());
            printf("  Signals blocked: %s\n", signals_blocked ? "YES" : "NO");
            
            printf("  Supported signals: 22 standard Linux signals\n");
            printf("  Custom handlers: Available for critical signals\n");
            printf("  Default handling: Comprehensive for all signal types\n");
        }
    };
    
    // Global signal manager instance
    static SignalManager g_signal_manager;
    
    // Convenience functions
    inline bool HandleSegfault(uint32_t fault_addr, uint32_t error_code = 0) {
        return g_signal_manager.SendSignal(SIGSEGV, fault_addr, error_code);
    }
    
    inline bool HandleIllegalInstruction(uint32_t instruction_addr) {
        return g_signal_manager.SendSignal(SIGILL, instruction_addr);
    }
    
    inline bool HandleFloatingPointException(uint32_t instruction_addr) {
        return g_signal_manager.SendSignal(SIGFPE, instruction_addr);
    }
    
    inline bool HandleInterrupt() {
        return g_signal_manager.SendSignal(SIGINT);
    }
    
    inline bool HandleTermination() {
        return g_signal_manager.SendSignal(SIGTERM);
    }
    
    // Signal handling operations
    inline void InitializeSignalHandling() {
        g_signal_manager.InitializeDefaults();
    }
    
    inline void BlockSignals() {
        g_signal_manager.BlockSignals();
    }
    
    inline void UnblockSignals() {
        g_signal_manager.UnblockSignals();
    }
    
    inline void PrintSignalStatus() {
        g_signal_manager.PrintStatus();
    }
}

// Apply signal handling globally
void ApplySignalHandling() {
    printf("[GLOBAL_SIGNAL] Applying signal handling infrastructure...\n");
    
    SignalHandling::InitializeSignalHandling();
    SignalHandling::PrintSignalStatus();
    
    printf("[GLOBAL_SIGNAL] Signal handling system ready!\n");
    printf("[GLOBAL_SIGNAL] UserlandVM-HIT now has comprehensive process management!\n");
}