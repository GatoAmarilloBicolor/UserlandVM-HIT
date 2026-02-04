#!/bin/bash

# UserlandVM-HIT Colored Output Module
# Provides harmonized colors for system, user, debug, info, warning, and error messages

# Color definitions
export COLOR_DEBUG='\033[38;5;57m'      # Cool blue - technical debug info
export COLOR_INFO='\033[38;5;82m'       # Light green - informative program messages  
export COLOR_USER='\033[38;5;208m'      # Bright orange - user program output
export COLOR_SYSTEM='\033[38;5;39m'     # Deep blue - system/kernel messages
export COLOR_WARNING='\033[38;5;11m'   # Bright yellow - non-critical warnings
export COLOR_ERROR='\033[38;5;196m'     # Bright red - critical errors
export COLOR_RESET='\033[0m'           # Reset to default

# Color logging functions
log_debug() {
    echo -e "${COLOR_DEBUG}[DEBUG]${COLOR_RESET} $1"
}

log_info() {
    echo -e "${COLOR_INFO}[INFO]${COLOR_RESET} $1"
}

log_user() {
    echo -e "${COLOR_USER}[USER]${COLOR_RESET} $1"
}

log_system() {
    echo -e "${COLOR_SYSTEM}[SYSTEM]${COLOR_RESET} $1"
}

log_warning() {
    echo -e "${COLOR_WARNING}[WARNING]${COLOR_RESET} $1"
}

log_error() {
    echo -e "${COLOR_ERROR}[ERROR]${COLOR_RESET} $1"
}

# Special formatting for specific message types
log_write_syscall() {
    echo -e "${COLOR_SYSTEM}[SYSCALL]${COLOR_RESET} write(fd=$1, buffer=$2, size=$3)"
}

log_program_start() {
    echo -e "${COLOR_INFO}[EXECUTION]${COLOR_RESET} Starting: $1"
}

log_program_result() {
    local exit_code=$1
    if [ $exit_code -eq 0 ]; then
        echo -e "${COLOR_USER}[PROGRAM]${COLOR_RESET} Exit code: ${exit_code} ✓"
    else
        echo -e "${COLOR_USER}[PROGRAM]${COLOR_RESET} Exit code: ${exit_code} ✗"
    fi
}

log_elf_info() {
    echo -e "${COLOR_DEBUG}[ELF]${COLOR_RESET} $1"
}

log_symbol_info() {
    echo -e "${COLOR_DEBUG}[SYMBOL]${COLOR_RESET} $1"
}

log_relocation_info() {
    echo -e "${COLOR_DEBUG}[RELOC]${COLOR_RESET} $1"
}

log_memory_info() {
    echo -e "${COLOR_DEBUG}[MEMORY]${COLOR_RESET} $1"
}

log_cpu_info() {
    echo -e "${COLOR_DEBUG}[CPU]${COLOR_RESET} $1"
}

log_syscall_info() {
    echo -e "${COLOR_SYSTEM}[SYSCALL]${COLOR_RESET} $1"
}

# Progress indicator with colors
show_progress_colored() {
    local current=$1
    local total=$2
    local desc=$3
    local percent=$((current * 100 / total))
    local filled=$((percent / 2))
    local empty=$((50 - filled))
    
    printf "\r${COLOR_DEBUG}[PROGRESS]${COLOR_RESET} %s [%s%s] %d%% (%d/%d)" "$desc" "$(printf '%*s' $filled | tr ' ' '=')" "$(printf '%*s' $empty)" "$percent" "$current" "$total"
    printf "${COLOR_RESET}"
    
    if [ $current -eq $total ]; then
        echo ""
        log_info "Progress completed!"
    fi
}

# Section headers with colors
print_section() {
    echo ""
    echo -e "${COLOR_INFO}=== $1 ===${COLOR_RESET}"
    echo ""
}

print_subsection() {
    echo -e "${COLOR_DEBUG}--- $1 ---${COLOR_RESET}"
}

# Success/failure indicators
log_success() {
    echo -e "${COLOR_INFO}✅ SUCCESS: $1${COLOR_RESET}"
}

log_failure() {
    echo -e "${COLOR_ERROR}❌ FAILED: $1${COLOR_RESET}"
}

log_partial_success() {
    echo -e "${COLOR_WARNING}⚠️ PARTIAL: $1${COLOR_RESET}"
}

# Color test function
test_colors() {
    echo "Testing UserlandVM-HIT Color Scheme:"
    echo ""
    echo -e "${COLOR_DEBUG}[DEBUG]${COLOR_RESET} Debug information (technical details)"
    echo -e "${COLOR_INFO}[INFO]${COLOR_RESET} Information messages (program flow)"
    echo -e "${COLOR_USER}[USER]${COLOR_RESET} User program output"
    echo -e "${COLOR_SYSTEM}[SYSTEM]${COLOR_RESET} System/kernel messages"
    echo -e "${COLOR_WARNING}[WARNING]${COLOR_RESET} Non-critical warnings"
    echo -e "${COLOR_ERROR}[ERROR]${COLOR_RESET} Critical errors"
    echo ""
    echo "Sample mixed output:"
    log_debug "Loading ELF header"
    log_info "Program starting"
    log_user "Hello from Haiku!"
    log_system "Kernel syscall intercepted"
    log_warning "Feature not implemented yet"
    log_error "Memory access violation"
    echo ""
}