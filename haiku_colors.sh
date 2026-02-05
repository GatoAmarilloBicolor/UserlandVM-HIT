#!/bin/bash

# UserlandVM-HIT - Haiku OS Native Colored Output System
# Harmonized color scheme for Haiku OS debugging and development

# Haiku-inspired color palette for optimal Haiku desktop experience
export COLOR_HAIKU_DEBUG='\033[38;5;17m'    # Deep Haiku blue - technical debug info
export COLOR_HAIKU_INFO='\033[38;5;71m'     # Bright Haiku green - informational messages
export COLOR_HAIKU_SUCCESS='\033[38;5;40m'   # Success green - successful operations
export COLOR_HAIKU_WARNING='\033[38;5;214m'  # Orange warning - Haiku-style warnings
export COLOR_HAIKU_ERROR='\033[38;5;196m'    # Bright red - critical errors
export COLOR_HAIKU_USER='\033[38;5;11m'      # Bright yellow - user program output
export COLOR_HAIKU_SYSTEM='\033[38;5;20m'    # Haiku deep blue - system operations
export COLOR_HAIKU_GUI='\033[38;5;135m'       # Purple - GUI operations
export COLOR_HAIKU_THREAD='\033[38;5;27m'     # Magenta - threading operations
export COLOR_HAIKU_MEMORY='\033[38;5;51m'    # Cyan - memory operations
export COLOR_HAIKU_FILE='\033[38;5;240m'     # Bright cyan - file operations
export COLOR_HAIKU_PERF='\033[38;5;13m'       # Bright green - performance metrics
export COLOR_HAIKU_OPT='\033[38;5;203m'      # Light green - optimization info
export COLOR_HAIKU_RESET='\033[0m'              # Reset to default

# Logging functions with Haiku styling
haiku_debug() {
    echo -e "${COLOR_HAIKU_DEBUG}[HAIKU DEBUG]${COLOR_RESET} $1"
}

haiku_info() {
    echo -e "${COLOR_HAIKU_INFO}[HAIKU INFO]${COLOR_RESET} $1"
}

haiku_success() {
    echo -e "${COLOR_HAIKU_SUCCESS}[HAIKU ✓]${COLOR_RESET} $1"
}

haiku_warning() {
    echo -e "${COLOR_HAIKU_WARNING}[HAIKU ⚠]${COLOR_RESET} $1"
}

haiku_error() {
    echo -e "${COLOR_HAIKU_ERROR}[HAIKU ✗]${COLOR_RESET} $1"
}

haiku_user() {
    echo -e "${COLOR_HAIKU_USER}[HAIKU OUTPUT]${COLOR_RESET} $1"
}

haiku_system() {
    echo -e "${COLOR_HAIKU_SYSTEM}[HAIKU SYSTEM]${COLOR_RESET} $1"
}

haiku_gui() {
    echo -e "${COLOR_HAIKU_GUI}[HAIKU GUI]${COLOR_RESET} $1"
}

haiku_thread() {
    echo -e "${COLOR_HAIKU_THREAD}[HAIKU THREAD]${COLOR_RESET} $1"
}

haiku_memory() {
    echo -e "${COLOR_HAIKU_MEMORY}[HAIKU MEMORY]${COLOR_RESET} $1"
}

haiku_file() {
    echo -e "${COLOR_HAIKU_FILE}[HAIKU FILE]${COLOR_RESET} $1"
}

haiku_perf() {
    echo -e "${COLOR_HAIKU_PERF}[HAIKU PERF]${COLOR_RESET} $1"
}

haiku_opt() {
    echo -e "${COLOR_HAIKU_OPT}[HAIKU OPT]${COLOR_RESET} $1"
}

# Specialized logging for Haiku components
haiku_area() {
    echo -e "${COLOR_HAIKU_MEMORY}[HAIKU AREA]${COLOR_RESET} $1"
}

haiku_thread_id() {
    echo -e "${COLOR_HAIKU_THREAD}[HAIKU TID:${1}]${COLOR_RESET} $2"
}

haiku_syscall() {
    local syscall=$1
    shift
    echo -e "${COLOR_HAIKU_SYSTEM}[HAIKU SYSCALL:${COLOR_HAIKU_RESET} ${COLOR_HAIKU_DEBUG}${syscall}${COLOR_RESET} ${COLOR_HAIKU_SYSTEM}$*${COLOR_RESET} $@"
}

haiku_gui_window() {
    local action=$1
    shift
    echo -e "${COLOR_HAIKU_GUI}[HAIKU WINDOW]${COLOR_RESET} ${action}: $@"
}

haiku_gui_draw() {
    local operation=$1
    shift
    echo -e "${COLOR_HAIKU_GUI}[HAIKU DRAW]${COLOR_RESET} ${operation}: $@"
}

# Progress indicators with Haiku styling
haiku_progress() {
    local current=$1
    local total=$2
    local desc=$3
    local percent=$((current * 100 / total))
    local filled=$((percent / 2))
    local empty=$((50 - filled))
    
    printf "\r${COLOR_HAIKU_INFO}[HAIKU PROGRESS]${COLOR_RESET} %s [%s%s] %d%% (%d/%d)" "$desc" \
           "$(printf '%*s' $filled | tr ' ' '=)" \
           "$(printf '%*s' $empty | tr ' ' '-')" \
           "$percent" "$current" "$total"
    printf "${COLOR_RESET}"
    
    if [ $current -eq $total ]; then
        echo ""
        haiku_success "Progress completed: $desc"
    fi
}

haiku_spinner() {
    local pid=$1
    local spin_chars="⣾⣽⣽⣾"
    local i=0
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r${COLOR_HAIKU_INFO}[HAIKU WORKING]${COLOR_RESET} ${spin_chars:$i:1}"
        i=$(( (i + 1) % 4))
        sleep 0.1
    done
    printf "\r${COLOR_HAIKU_SUCCESS}[HAIKU ✓]${COLOR_RESET} Operation completed\n"
}

# Error handling with Haiku styling
haiku_error_report() {
    local error_code=$1
    local error_message=$2
    local error_context=$3
    
    echo -e "${COLOR_HAIKU_ERROR}[HAIKU ERROR]${COLOR_RESET} Code: ${COLOR_HAIKU_DEBUG}${error_code}${COLOR_RESET}"
    echo -e "${COLOR_HAIKU_ERROR}[HAIKU ERROR]${COLOR_RESET} Message: ${error_message}"
    echo -e "${COLOR_HAIKU_ERROR}[HAIKU ERROR]${COLOR_RESET} Context: ${error_context}"
}

# Memory debugging
haiku_memory_dump() {
    local address=$1
    local size=$2
    local format=${3:-"hex"}
    
    if [ "$format" = "hex" ]; then
        echo -e "${COLOR_HAIKU_MEMORY}[HAIKU MEMORY DUMP]${COLOR_RESET} Address: ${COLOR_HAIKU_DEBUG}${address}${COLOR_RESET}, Size: ${COLOR_HAIKU_DEBUG}${size}${COLOR_RESET}"
    elif [ "$format" = "ascii" ]; then
        echo -e "${COLOR_HAIKU_MEMORY}[HAIKU MEMORY ASCII]${COLOR_RESET} Address: ${COLOR_HAIKU_DEBUG}${address}${COLOR_RESET}, Size: ${COLOR_HAIKU_DEBUG}${size}${COLOR_RESET}"
    fi
}

# Performance metrics
haiku_perf_start() {
    export HAIKU_PERF_START=$(date +%s%N)
    haiku_debug "Performance measurement started"
}

haiku_perf_end() {
    local operation=$1
    local end_time=$(date +%s%N)
    local duration=$((end_time - HAIKU_PERF_START))
    
    haiku_perf "${operation}: ${duration}μs"
    unset HAIKU_PERF_START
}

haiku_perf_memory() {
    local operation=$1
    local before=$2
    local after=$3
    
    local diff=$((after - before))
    haiku_memory "Memory ${operation}: ${diff} bytes"
}

# System state reporting
haiku_system_report() {
    echo -e "${COLOR_HAIKU_SYSTEM}=== Haiku System Report ====${COLOR_RESET}"
    echo -e "${COLOR_HAIKU_SYSTEM}System: $(uname -a)"
    echo -e "${COLOR_HAIKU_SYSTEM}Haiku: $(getsysinfo -q 2>/dev/null || echo 'Unknown')"
    echo -e "${COLOR_HAIKU_SYSTEM}Memory: $(free -h | head -1)"
    echo -e "${COLOR_HAIKU_SYSTEM}Load: $(uptime)"
    echo -e "${COLOR_HAIKU_SYSTEM}=== End Report ====${COLOR_RESET}"
}

# Debugging utilities
haiku_debug_section() {
    local section=$1
    echo ""
    echo -e "${COLOR_HAIKU_DEBUG}=== Haiku Debug: ${section} ===${COLOR_RESET}"
}

haiku_debug_subsection() {
    local subsection=$1
    echo -e "${COLOR_HAIKU_INFO}[HAIKU DEBUG]${COLOR_RESET} → ${subsection}"
}

haiku_debug_value() {
    local name=$1
    local value=$2
    echo -e "${COLOR_HAIKU_DEBUG}[HAIKU DEBUG]${COLOR_RESET} ${name}: ${COLOR_HAIKU_SUCCESS}${value}${COLOR_RESET}"
}

# Cleanup and reset
haiku_reset_colors() {
    printf "${COLOR_HAIKU_RESET}"
}

# Test the color scheme
haiku_test_colors() {
    echo ""
    echo -e "${COLOR_HAIKU_DEBUG}Testing Haiku Color Scheme${COLOR_RESET}"
    echo ""
    haiku_debug "Debug information"
    haiku_info "Informational messages"
    haiku_success "Successful operations"
    haiku_warning "Warning messages"
    haiku_error "Critical errors"
    haiku_user "User program output"
    haiku_system "System operations"
    haiku_gui "GUI operations"
    haiku_thread "Threading operations"
    haiku_memory "Memory operations"
    haiku_file "File operations"
    haiku_perf "Performance metrics"
    haiku_opt "Optimization info"
    echo ""
    haiku_reset_colors
    echo "Haiku color scheme test completed!"
}

# Haiku-themed startup
haiku_startup() {
    echo ""
    echo -e "${COLOR_HAIKU_SUCCESS}╔════════════════════════════════════════════════════════╗${COLOR_RESET}"
    echo -e "${COLOR_HAIKU_SUCCESS}║${COLOR_RESET}                    ${COLOR_HAIKU_INFO}UserlandVM-HIT${COLOR_RESET}                    ${COLOR_HAIKU_SUCCESS}║${COLOR_RESET}"
    echo -e "${COLOR_HAIKU_SUCCESS}║${COLOR_RESET}     ${COLOR_HAIKU_INFO}Haiku OS Native Emulator${COLOR_RESET}     ${COLOR_HAIKU_SUCCESS}║${COLOR_RESET}"
    echo -e "${COLOR_HAIKU_SUCCESS}║${COLOR_RESET}                    ${COLOR_HAIKU_INFO}Version 1.0.0${COLOR_RESET}                    ${COLOR_HAIKU_SUCCESS}║${COLOR_RESET}"
    echo -e "${COLOR_HAIKU_SUCCESS}╚══════════════════════════════════════════════════════════╝${COLOR_RESET}"
    echo ""
    haiku_info "Native Haiku optimization enabled"
    haiku_info "Harmonized color scheme active"
    echo ""
}

# Export all functions for sourcing
export -f haiku_debug haiku_info haiku_success haiku_warning haiku_error haiku_user haiku_system
export -f haiku_gui haiku_thread haiku_memory haiku_file haiku_perf haiku_opt
export -f haiku_area haiku_thread_id haiku_syscall haiku_gui_window haiku_gui_draw
export -f haiku_progress haiku_spinner haiku_error_report haiku_memory_dump
export -f haiku_perf_start haiku_perf_end haiku_perf_memory haiku_system_report
export -f haiku_debug_section haiku_debug_subsection haiku_debug_value
export -f haiku_reset_colors haiku_test_colors haiku_startup