#include "include/HaikuLogging.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <sstream>

///////////////////////////////////////////////////////////////////////////
// HaikuLogger Implementation
///////////////////////////////////////////////////////////////////////////

void HaikuLogger::SetComponentEnabled(const std::string& component, bool enabled) {
    std::lock_guard<std::mutex> lock(log_mutex);
    component_enabled[component] = enabled;
}

std::string HaikuLogger::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string HaikuLogger::GetLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default:              return "?????";
    }
}

bool HaikuLogger::ShouldLog(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(current_level);
}

void HaikuLogger::Log(LogLevel level, const std::string& component, const char* format, va_list args) {
    if (!ShouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // Check if component is enabled
    if (!component_enabled.empty()) {
        auto it = component_enabled.find(component);
        if (it != component_enabled.end() && !it->second) {
            return;
        }
    }
    
    // Print timestamp if enabled
    if (timestamp_enabled) {
        printf("[%s] ", GetTimestamp().c_str());
    }
    
    // Print level and component
    printf("[%-5s][%-10s] ", GetLevelString(level).c_str(), component.c_str());
    
    // Print formatted message
    vprintf(format, args);
    printf("\n");
    fflush(stdout);
}

void HaikuLogger::Log(LogLevel level, const char* format, va_list args) {
    if (!ShouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // Print timestamp if enabled
    if (timestamp_enabled) {
        printf("[%s] ", GetTimestamp().c_str());
    }
    
    // Print level
    printf("[%-5s] ", GetLevelString(level).c_str());
    
    // Print formatted message
    vprintf(format, args);
    printf("\n");
    fflush(stdout);
}

void HaikuLogger::Debug(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::DEBUG, component, format, args);
    va_end(args);
}

void HaikuLogger::Info(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::INFO, component, format, args);
    va_end(args);
}

void HaikuLogger::Warn(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::WARN, component, format, args);
    va_end(args);
}

void HaikuLogger::Error(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::ERROR, component, format, args);
    va_end(args);
}

void HaikuLogger::Debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::DEBUG, format, args);
    va_end(args);
}

void HaikuLogger::Info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::INFO, format, args);
    va_end(args);
}

void HaikuLogger::Warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::WARN, format, args);
    va_end(args);
}

void HaikuLogger::Error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Log(LogLevel::ERROR, format, args);
    va_end(args);
}

void HaikuLogger::LogDebug(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    GetInstance().Log(LogLevel::DEBUG, component, format, args);
    va_end(args);
}

void HaikuLogger::LogInfo(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    GetInstance().Log(LogLevel::INFO, component, format, args);
    va_end(args);
}

void HaikuLogger::LogWarn(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    GetInstance().Log(LogLevel::WARN, component, format, args);
    va_end(args);
}

void HaikuLogger::LogError(const std::string& component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    GetInstance().Log(LogLevel::ERROR, component, format, args);
    va_end(args);
}
