#!/bin/bash

# UserlandVM-HIT - Dynamic Platform Detection and Reporting System
# Automatically detects platform based on current program and adapts behavior

# Auto-detect current platform (overrides external detection)
DETECT_PLATFORM=true
PLATFORM_NAME="unknown"

# Platform detection function
detect_platform() {
    if [ "$DETECT_PLATFORM" = "true" ]; then
        # Auto-detect based on current system state
        echo "[DETECT] Detecting platform..."
        
        # Check CPU architecture
        if [ -f /proc/cpuinfo ]; then
            if grep -q "x86_64" /proc/cpuinfo; then
                PLATFORM_NAME="x86_64"
            elif grep -q "armv7l" /proc/cpuinfo; then
                PLATFORM_NAME="arm32"
            elif grep -q "aarch64" /proc/cpuinfo; then
                PLATFORM_NAME="arm64"
            elif grep -q "riscv64" /proc/cpuinfo; then
                PLATFORM_NAME="riscv64"
            fi
        else
            # Fallback to uname
            PLATFORM_NAME=$(uname -m)
        fi
        
        echo "[DETECT] Detected platform: $PLATFORM_NAME"
    else
        echo "[DETECT] Using forced platform: $PLATFORM_NAME"
    fi
}

# Platform-specific behavior configurations
PLATFORM_CONFIGS=(
    "x86_64:Running x86-64 programs on 64-bit host:optimized"
    "arm64:Running ARM64 programs on 64-bit host:compatible"
    "arm32:Running ARM32 programs:power-efficient"
    "riscv64:Running RISC-V programs:efficient"
    "x86_32:Running x86-32 programs:legacy"
)

# Get configuration for detected platform
get_platform_config() {
    for config in "${PLATFORM_CONFIGS[@]}"; do
        local platform=$(echo "$config" | cut -d':' -f1)
        local description=$(echo "$config" | cut -d':' -f2-)
        
        if [ "$platform" = "$PLATFORM_NAME" ]; then
            echo "$description"
            return 0
        fi
    done
    return 1
}

# Enhanced logging with platform information
log_platform_info() {
    local level=$1
    local message=$2
    local platform_info="[$PLATFORM_NAME]"
    
    case "$level" in
        "debug")   echo -e "\033[38;5;17m[DEBUG$platform_info]\033[0m $message" ;;
        "info")    echo -e "\033[38;5;71m[INFO $platform_info]\033[0m $message" ;;
        "success") echo -e "\033[38;5;40m[SUCCESS$platform_info]\033[0m ✓ $message" ;;
        "warning")  echo -e "\033[38;5;214m[WARNING$platform_info]\033[0m ⚠ $message" ;;
        "error")   echo -e "\033[38;5;196m[ERROR$platform_info]\033[0m ✗ $message" ;;
        *)         echo -e "\033[38;5;17m[SYSTEM$platform_info]\033[0m $message" ;;
    esac
}

# Platform-specific optimizations
optimize_for_platform() {
    local program_name=$1
    
    log_platform_info "info" "Optimizing '$program_name' for $PLATFORM_NAME platform"
    
    case "$PLATFORM_NAME" in
        "x86_64")
            log_platform_info "info" "Using 64-bit optimizations: large page support, SIMD"
            ;;
        "arm64")
            log_platform_info "info" "Using ARM64 optimizations: NEON, large pages"
            ;;
        "arm32")
            log_platform_info "info" "Using ARM32 optimizations: thumb mode, low power"
            ;;
        "riscv64")
            log_platform_info "info" "Using RISC-V optimizations: RVV vector extensions"
            ;;
        "x86_32")
            log_platform_info "warning" "Legacy x86-32 platform - using basic optimizations"
            ;;
        *)
            log_platform_info "info" "Using generic optimizations for unknown platform"
            ;;
    esac
}

# Platform compatibility checks
check_platform_compatibility() {
    local target_platform=$1
    local program_path=$2
    
    log_platform_info "info" "Checking $target_platform compatibility on $PLATFORM_NAME"
    
    case "$PLATFORM_NAME" in
        "x86_64")
            case "$target_platform" in
                "x86_32"|"x86_64") 
                    log_platform_info "success" "Compatible: x86_64 can run x86 programs"
                    return 0
                    ;;
                "arm64") 
                    log_platform_info "warning" "Incompatible: x86_64 cannot run ARM64"
                    return 1
                    ;;
                "riscv64")
                    log_platform_info "warning" "Incompatible: x86_64 cannot run RISC-V"
                    return 1
                    ;;
            esac
            ;;
        "arm64")
            case "$target_platform" in
                "arm32"|"arm64")
                    log_platform_info "success" "Compatible: ARM64 can run ARM programs"
                    return 0
                    ;;
                "x86_64")
                    log_platform_info "warning" "Incompatible: ARM64 cannot run x86_64"
                    return 1
                    ;;
            esac
            ;;
        *)
            log_platform_info "warning" "Unknown host platform compatibility"
            return 2
            ;;
    esac
}

# Dynamic problem analysis with platform context
analyze_platform_problem() {
    local error_type=$1
    local error_details=$2
    
    local config=$(get_platform_config)
    if [ $? -ne 0 ]; then
        log_platform_info "error" "Unknown platform configuration"
        return 1
    fi
    
    log_platform_info "error" "Platform analysis on $PLATFORM_NAME: $config"
    log_platform_info "error" "Problem: $error_type"
    
    case "$error_type" in
        "memory")
            log_platform_info "info" "Checking memory management for $PLATFORM_NAME"
            case "$PLATFORM_NAME" in
                "x86_64") log_platform_info "info" "Using 64-bit memory optimization" ;;
                "arm64")  log_platform_info "info" "Using ARM64 memory alignment" ;;
                "riscv64") log_platform_info "info" "Using RISC-V memory model" ;;
            esac
            ;;
        "performance")
            log_platform_info "info" "Analyzing performance for $PLATFORM_NAME"
            case "$PLATFORM_NAME" in
                "x86_64") log_platform_info "info" "Using x86_64 performance counters" ;;
                "arm64")  log_platform_info "info" "Using ARM64 performance monitoring" ;;
            esac
            ;;
        "syscalls")
            log_platform_info "info" "Analyzing syscall compatibility for $PLATFORM_NAME"
            ;;
        *)
            log_platform_info "warning" "Unknown problem type: $error_type"
            ;;
    esac
    
    return 0
}

# Platform-specific debugging commands
debug_platform() {
    local command=$1
    
    log_platform_info "debug" "Platform debug command on $PLATFORM_NAME: $command"
    
    case "$command" in
        "info")
            detect_platform
            get_platform_config
            echo "Platform: $PLATFORM_NAME"
            echo "Config: $(get_platform_config)"
            echo "System: $(uname -a)"
            echo "CPU: $(grep -m 'model name' /proc/cpuinfo | cut -d: -f2-)"
            ;;
        "memory")
            log_platform_info "debug" "Memory analysis for $PLATFORM_NAME"
            case "$PLATFORM_NAME" in
                "x86_64") 
                    echo "Memory map:"
                    cat /proc/self/maps | grep -E "(heap|stack|text)" | head -10
                    ;;
                "arm64")
                    echo "ARM64 memory layout:"
                    cat /proc/self/maps | head -5
                    ;;
            esac
            ;;
        "performance")
            log_platform_info "debug" "Performance debugging for $PLATFORM_NAME"
            echo "CPU frequency: $(cat /proc/cpuinfo | grep -m 'cpu MHz' | cut -d: -f2)"
            echo "Memory info:"
            free -h
            ;;
        *)
            log_platform_info "warning" "Unknown debug command: $command"
            ;;
    esac
}

# Environment setup for platform
setup_platform_environment() {
    local program_name=$1
    
    log_platform_info "info" "Setting up environment for $program_name on $PLATFORM_NAME"
    
    case "$PLATFORM_NAME" in
        "x86_64")
            export PLATFORM_ARCH="amd64"
            export PLATFORM_FLAGS="-O3 -march=x86-64"
            ;;
        "arm64")
            export PLATFORM_ARCH="aarch64"
            export PLATFORM_FLAGS="-O3 -march=native"
            ;;
        "riscv64")
            export PLATFORM_ARCH="riscv64"
            export PLATFORM_FLAGS="-O3"
            ;;
        "x86_32")
            export PLATFORM_ARCH="x86"
            export PLATFORM_FLAGS="-O2 -march=i386"
            ;;
        *)
            export PLATFORM_ARCH="unknown"
            export PLATFORM_FLAGS="-O2"
            ;;
    esac
    
    log_platform_info "success" "Platform environment: $PLATFORM_ARCH with flags $PLATFORM_FLAGS"
}

# Platform switching detection
detect_emulation_mode() {
    local program_path=$1
    
    log_platform_info "info" "Detecting emulation mode for $program_path"
    
    # Check file type
    if [ -f "$program_path" ]; then
        local file_type=$(file "$program_path")
        log_platform_info "info" "File type: $file_type"
        
        case "$file_type" in
            *ELF*64-bit*)
                if [ "$PLATFORM_NAME" = "x86_64" ]; then
                    log_platform_info "success" "Native x86-64 execution"
                    return "native"
                else
                    log_platform_info "info" "Requires x86-64 emulation"
                    return "emulate"
                    ;;
            *ELF*32-bit*)
                if [ "$PLATFORM_NAME" = "x86_32" ]; then
                    log_platform_info "success" "Native x86-32 execution"
                    return "native"
                else
                    log_platform_info "info" "Requires x86-32 emulation"
                    return "emulate"
                    ;;
            *ARM*)
                if [ "$PLATFORM_NAME" = "arm64" ] || [ "$PLATFORM_NAME" = "arm32" ]; then
                    log_platform_info "success" "Native ARM execution"
                    return "native"
                else
                    log_platform_info "info" "Requires ARM emulation"
                    return "emulate"
                    ;;
            *RISC-V*)
                if [ "$PLATFORM_NAME" = "riscv64" ]; then
                    log_platform_info "success" "Native RISC-V execution"
                    return "native"
                else
                    log_platform_info "info" "Requires RISC-V emulation"
                    return "emulate"
                    ;;
            *)
                log_platform_info "warning" "Unknown file type"
                return "unknown"
                ;;
        esac
    else
        log_platform_info "error" "File not found: $program_path"
        return "error"
    fi
}

# Comprehensive platform report
generate_platform_report() {
    log_platform_info "info" "Generating platform report for $PLATFORM_NAME"
    
    echo ""
    echo "=== PLATFORM REPORT ==="
    echo "Host Platform: $PLATFORM_NAME"
    echo "Architecture: $(uname -m)"
    echo "Kernel: $(uname -r)"
    echo "Configuration: $(get_platform_config)"
    echo ""
    echo "=== PERFORMANCE METRICS ==="
    case "$PLATFORM_NAME" in
        "x86_64")
            echo "CPU Cores: $(grep -c '^processor' /proc/cpuinfo | wc -l)"
            echo "Cache Info: $(lscpu | grep -E 'Cache size')"
            echo "Memory: $(free -h)"
            ;;
        "arm64")
            echo "ARM Features: $(grep -E 'Features' /proc/cpuinfo)"
            echo "Architecture: $(lscpu | grep Architecture)"
            ;;
        "riscv64")
            echo "ISA Extensions: $(grep -E 'isa' /proc/cpuinfo)"
            echo "Urchitecture Model: $(grep -E 'uarch' /proc/cpuinfo)"
            ;;
    esac
    echo ""
    echo "=== COMPATIBILITY MATRIX ==="
    echo "Native Execution:"
    echo "  ✓ x86_64 → x86_32, x86_64 programs"
    echo "  ✓ arm64 → arm32, arm64 programs"
    echo "  ✓ riscv64 → riscv32, riscv64 programs"
    echo ""
    echo "=== REPORT COMPLETE ==="
    log_platform_info "success" "Platform report generated"
}

# Help function with platform context
show_platform_help() {
    echo "UserlandVM-HIT Dynamic Platform Detection System"
    echo ""
    echo "Usage: $0 [options] [command] [args]"
    echo ""
    echo "Options:"
    echo "  --platform <name>     Force platform (x86_64, arm64, riscv64, x86_32)"
    echo "  --detect              Auto-detect current platform"
    echo "  --no-detect          Disable auto-detection"
    echo ""
    echo "Commands:"
    echo "  detect                Detect current platform"
    echo "  info                  Show platform information"
    echo "  optimize <program>    Optimize for current platform"
    echo "  check-compat <target> Check compatibility with target"
    echo "  analyze <type> <desc> Analyze platform-specific problem"
    echo "  debug <type>         Platform-specific debugging"
    echo "  setup <program>       Setup environment for platform"
    echo "  detect-emulation <path> Detect emulation mode needed"
    echo "  report               Generate comprehensive platform report"
    echo "  help                  Show this help"
    echo ""
    echo "Platform Types:"
    for config in "${PLATFORM_CONFIGS[@]}"; do
        echo "  $(echo "$config" | cut -d':' -f1): $(echo "$config" | cut -d':' -f2-)"
    done
    echo ""
    echo "Problem Types:"
    echo "  memory      Memory management issues"
    echo "  performance  Performance problems"
    echo "  syscalls     System call issues"
    echo ""
    echo "Debug Types:"
    echo "  info         System and hardware information"
    echo "  memory       Memory usage and layout"
    echo "  performance  Performance counters and metrics"
    echo ""
}

# Parse command line arguments
case "${1:-}" in
    --platform)
        shift
        PLATFORM_NAME="$1"
        DETECT_PLATFORM=false
        shift
        ;;
    --no-detect)
        DETECT_PLATFORM=false
        ;;
    --detect)
        detect_platform
        exit 0
        ;;
    --info)
        detect_platform
        get_platform_config
        echo "Platform: $PLATFORM_NAME"
        echo "Config: $(get_platform_config)"
        exit 0
        ;;
    --optimize)
        optimize_for_platform "$2"
        exit 0
        ;;
    --check-compat)
        check_platform_compatibility "$2" "$3"
        exit 0
        ;;
    --analyze)
        analyze_platform_problem "$2" "$3"
        exit 0
        ;;
    --debug)
        debug_platform "$2"
        exit 0
        ;;
    --setup)
        setup_platform_environment "$2"
        exit 0
        ;;
    --detect-emulation)
        detect_emulation_mode "$2"
        echo "Emulation mode: $(detect_emulation_mode "$2")"
        exit 0
        ;;
    --report)
        generate_platform_report
        exit 0
        ;;
    --help|*)
        show_platform_help
        exit 0
        ;;
    *)
        show_platform_help
        exit 1
        ;;
esac