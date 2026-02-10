#include "include/HaikuLogging.h"
#include <iostream>

///////////////////////////////////////////////////////////////////////////
// HaikuLogger Implementation
///////////////////////////////////////////////////////////////////////////

void HaikuLogger::SetComponentEnabled(const std::string& component, bool enabled) {
    std::lock_guard<std::mutex> lock(log_mutex);
    component_enabled[component] = enabled;
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

void HaikuLogger::Log(LogLevel level, const std::string& component, const char* format, va_list args) {
    if (!ShouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // Check if component is enabled
    auto it = component_enabled.find(component);
    if (it != component_enabled.end() && !it->second) {
        return;
    }
    
    // Build log message
    std::string message;
    if (timestamp_enabled) {
        message += GetTimestamp() + " ";
    }
    
    message += "[" + GetLevelString(level) + "] ";
    message += "[" + component + "] ";
    
    // Format the user message
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    message += buffer;
    
    // Output to stderr for errors, stdout for everything else
    if (level == LogLevel::ERROR) {
        fprintf(stderr, "%s\n", message.c_str());
        fflush(stderr);
    } else {
        fprintf(stdout, "%s\n", message.c_str());
        fflush(stdout);
    }
}

void HaikuLogger::Log(LogLevel level, const char* format, va_list args) {
    if (!ShouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // Build log message
    std::string message;
    if (timestamp_enabled) {
        message += GetTimestamp() + " ";
    }
    
    message += "[" + GetLevelString(level) + "] ";
    
    // Format the user message
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    message += buffer;
    
    // Output to stderr for errors, stdout for everything else
    if (level == LogLevel::ERROR) {
        fprintf(stderr, "%s\n", message.c_str());
        fflush(stderr);
    } else {
        fprintf(stdout, "%s\n", message.c_str());
        fflush(stdout);
    }
}

std::string HaikuLogger::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&time_t));
    
    std::string timestamp = buffer;
    timestamp += "." + std::to_string(ms.count());
    
    return timestamp;
}

std::string HaikuLogger::GetLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool HaikuLogger::ShouldLog(LogLevel level) const {
    return level >= current_level;
}