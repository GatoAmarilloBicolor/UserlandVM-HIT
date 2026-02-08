#!/bin/bash
# UserlandVM-HIT Launcher Script
# Detects platform and executes binary with appropriate method
# Author: Launcher Script 2026-02-07

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BINARY_PATH=""
ENTRY_POINT=""
STACK_POINTER="0x7FFFF000"
VERBOSE=false
EXECUTION_MODE=""

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to detect system information
detect_system() {
    print_info "Detecting system information..."
    
    # Detect architecture
    ARCH=$(uname -m)
    print_info "System architecture: $ARCH"
    
    # Detect endianness
    ENDIANESS=$(echo -n I | od -to2 | head -1 | awk '{print $6}' | sed 's/^1/little/;s/^0/big/')
    print_info "Endianness: $ENDIANESS"
    
    # Detect CPU features
    if [ -f /proc/cpuinfo ]; then
        if grep -q "sse" /proc/cpuinfo; then
            print_info "SSE support: Yes"
        else
            print_info "SSE support: No"
        fi
        
        if grep -q "avx" /proc/cpuinfo; then
            print_info "AVX support: Yes"
        else
            print_info "AVX support: No"
        fi
    fi
    
    # Check pointer size (32 vs 64 bit)
    POINTER_SIZE=$(getconf LONG_BIT)
    print_info "Pointer size: $POINTER_SIZE bits"
}

# Function to validate binary file
validate_binary() {
    if [ -z "$BINARY_PATH" ]; then
        print_error "No binary file specified"
        usage
        exit 1
    fi
    
    if [ ! -f "$BINARY_PATH" ]; then
        print_error "Binary file not found: $BINARY_PATH"
        exit 1
    fi
    
    # Check if binary is executable
    if [ ! -x "$BINARY_PATH" ]; then
        print_warning "Binary file is not executable, making it executable..."
        chmod +x "$BINARY_PATH"
    fi
    
    # Check binary format
    FILE_FORMAT=$(file "$BINARY_PATH")
    print_info "Binary format: $FILE_FORMAT"
    
    # Check if it's an ELF binary
    if echo "$FILE_FORMAT" | grep -q "ELF"; then
        ELF_CLASS=$(readelf -h "$BINARY_PATH" | grep "Class:" | awk '{print $2}')
        print_info "ELF class: $ELF_CLASS"
        
        ELF_DATA=$(readelf -h "$BINARY_PATH" | grep "Data:" | awk '{print $2}')
        print_info "ELF data: $ELF_DATA"
        
        ELF_MACHINE=$(readelf -h "$BINARY_PATH" | grep "Machine:" | awk '{print $2}')
        print_info "ELF machine: $ELF_MACHINE"
        
        ENTRY_POINT=$(readelf -h "$BINARY_PATH" | grep "Entry point address:" | awk '{print $4}')
        print_info "Entry point: 0x$ENTRY_POINT"
        
        # Set default entry point if not specified
        if [ -z "$ENTRY_POINT" ]; then
            ENTRY_POINT="0x$ENTRY_POINT"
        fi
    else
        print_error "Unsupported binary format. Expected ELF binary."
        exit 1
    fi
}

# Function to determine execution mode
determine_execution_mode() {
    print_info "Determining execution mode..."
    
    # Architecture detection
    SYSTEM_ARCH=$(uname -m)
    ELF_ARCH=""
    
    # Parse ELF machine
    case "$ELF_MACHINE" in
        "EM_386")
            ELF_ARCH="x86-32"
            ;;
        "EM_X86_64")
            ELF_ARCH="x86-64"
            ;;
        "EM_RISCV32")
            ELF_ARCH="riscv-32"
            ;;
        "EM_RISCV64")
            ELF_ARCH="riscv-64"
            ;;
        "EM_ARM")
            ELF_ARCH="arm-32"
            ;;
        "EM_AARCH64")
            ELF_ARCH="arm-64"
            ;;
        *)
            ELF_ARCH="unknown"
            ;;
    esac
    
    print_info "Binary architecture: $ELF_ARCH"
    print_info "System architecture: $SYSTEM_ARCH"
    
    # Determine execution mode
    if [ "$SYSTEM_ARCH" = "$ELF_ARCH" ]; then
        EXECUTION_MODE="native"
        print_success "Native execution mode selected"
    elif [ "$SYSTEM_ARCH" = "x86_64" ] && [ "$ELF_ARCH" = "i386" ]; then
        EXECUTION_MODE="emulated-32"
        print_success "32-bit emulation on x86-64 system selected"
    elif [ "$SYSTEM_ARCH" = "x86_64" ] && [ "$ELF_ARCH" = "x86_64" ]; then
        EXECUTION_MODE="native-64"
        print_success "Native 64-bit execution selected"
    elif [[ "$SYSTEM_ARCH" == *"x86"* ]] && [[ "$ELF_ARCH" == *"arm"* || "$ELF_ARCH" == *"riscv"* ]]; then
        EXECUTION_MODE="sysroot"
        print_success "Sysroot execution mode selected (x86 host, non-x86 binary)"
    elif [[ "$SYSTEM_ARCH" == *"arm"* ]] && [[ "$ELF_ARCH" == *"x86"* ]]; then
        EXECUTION_MODE="sysroot"
        print_success "Sysroot execution mode selected (ARM host, x86 binary)"
    else
        EXECUTION_MODE="unsupported"
        print_error "Unsupported architecture combination: $SYSTEM_ARCH -> $ELF_ARCH"
        exit 1
    fi
}

# Function to create execution environment
setup_environment() {
    print_info "Setting up execution environment..."
    
    # Create temporary directory for execution
    TEMP_DIR=$(mktemp -d)
    print_info "Created temporary directory: $TEMP_DIR"
    
    # Set up environment variables
    export USERLANDVM_TEMP_DIR="$TEMP_DIR"
    export USERLANDVM_EXECUTION_MODE="$EXECUTION_MODE"
    export USERLANDVM_VERBOSE="$VERBOSE"
    
    # Create log file
    LOG_FILE="$TEMP_DIR/userlandvm.log"
    export USERLANDVM_LOG_FILE="$LOG_FILE"
    
    print_info "Log file: $LOG_FILE"
    
    # Copy binary to temp directory if needed
    if [ "$EXECUTION_MODE" = "sysroot" ]; then
        cp "$BINARY_PATH" "$TEMP_DIR/binary"
        BINARY_PATH="$TEMP_DIR/binary"
        print_info "Binary copied to temporary directory"
    fi
}

# Function to execute binary
execute_binary() {
    print_info "Executing binary in $EXECUTION_MODE mode..."
    
    case "$EXECUTION_MODE" in
        "native"|"native-64")
            print_success "Executing natively: $BINARY_PATH"
            if [ "$VERBOSE" = "true" ]; then
                "$BINARY_PATH" 2>&1 | tee "$LOG_FILE"
            else
                "$BINARY_PATH"
            fi
            ;;
        "emulated-32")
            print_success "Executing with 32-bit emulation..."
            print_info "Loading userlandvm binary with emulation support"
            
            # For now, simulate with our detection system
            echo "32-bit emulation would be performed here"
            echo "Entry point: $ENTRY_POINT"
            echo "Stack pointer: $STACK_POINTER"
            ;;
        "sysroot")
            print_success "Executing with sysroot method..."
            print_info "Using QEMU or similar for cross-architecture execution"
            
            # Determine appropriate QEMU command
            case "$ELF_ARCH" in
                "i386")
                    QEMU_CMD="qemu-i386"
                    ;;
                "x86-64")
                    QEMU_CMD="qemu-x86_64"
                    ;;
                "arm")
                    QEMU_CMD="qemu-arm"
                    ;;
                "aarch64")
                    QEMU_CMD="qemu-aarch64"
                    ;;
                "riscv32")
                    QEMU_CMD="qemu-riscv32"
                    ;;
                "riscv64")
                    QEMU_CMD="qemu-riscv64"
                    ;;
                *)
                    QEMU_CMD="qemu-user-static"
                    ;;
            esac
            
            print_info "Using QEMU command: $QEMU_CMD"
            
            # Execute with QEMU
            if [ "$VERBOSE" = "true" ]; then
                $QEMU_CMD "$BINARY_PATH" 2>&1 | tee "$LOG_FILE"
            else
                $QEMU_CMD "$BINARY_PATH"
            fi
            ;;
        *)
            print_error "Unknown execution mode: $EXECUTION_MODE"
            exit 1
            ;;
    esac
}

# Function to cleanup
cleanup() {
    print_info "Cleaning up..."
    
    # Remove temporary directory if it exists
    if [ -n "$USERLANDVM_TEMP_DIR" ] && [ -d "$USERLANDVM_TEMP_DIR" ]; then
        rm -rf "$USERLANDVM_TEMP_DIR"
        print_info "Removed temporary directory: $USERLANDVM_TEMP_DIR"
    fi
    
    # Clear environment variables
    unset USERLANDVM_TEMP_DIR
    unset USERLANDVM_EXECUTION_MODE
    unset USERLANDVM_VERBOSE
    unset USERLANDVM_LOG_FILE
}

# Function to print usage
usage() {
    echo "UserlandVM-HIT Launcher"
    echo "Usage: $0 [options] <binary_path>"
    echo ""
    echo "Options:"
    echo "  -e, --entry-point <addr>    Set entry point address (default: auto-detect)"
    echo "  -s, --stack-pointer <addr> Set stack pointer (default: 0x7FFFF000)"
    echo "  -v, --verbose              Enable verbose output"
    echo "  -h, --help                 Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 ./hello_world"
    echo "  $0 -v -e 0x401000 ./hello_world"
    echo "  $0 -s 0x7FFE000 ./hello_world"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -e|--entry-point)
            ENTRY_POINT="$2"
            shift 2
            ;;
        -s|--stack-pointer)
            STACK_POINTER="$2"
            shift 2
            ;;
        -*)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
        *)
            if [ -z "$BINARY_PATH" ]; then
                BINARY_PATH="$1"
            else
                print_error "Multiple binary files specified"
                usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Main execution flow
main() {
    echo -e "${BLUE}UserlandVM-HIT Launcher${NC}"
    echo -e "${BLUE}========================${NC}"
    
    # Check if binary was specified
    if [ -z "$BINARY_PATH" ]; then
        print_error "No binary file specified"
        usage
        exit 1
    fi
    
    # Set up cleanup trap
    trap cleanup EXIT
    
    # System detection
    detect_system
    
    # Validate binary
    validate_binary
    
    # Determine execution mode
    determine_execution_mode
    
    # Setup environment
    setup_environment
    
    # Execute binary
    execute_binary
    
    print_success "Execution completed"
}

# Run main function
main