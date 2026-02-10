#ifndef HAIKU_LOGGING_H
#define HAIKU_LOGGING_H

#include <cstdio>
#include <cstdarg>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <ctime>

///////////////////////////////////////////////////////////////////////////
// Unified Logging System for Haiku UserlandVM
// Provides consistent, structured logging across all components
///////////////////////////////////////////////////////////////////////////

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class HaikuLogger {
public:
    static HaikuLogger& GetInstance() {
        static HaikuLogger instance;
        return instance;
    }
    
    // Configuration
    void SetLogLevel(LogLevel level) { current_level = level; }
    LogLevel GetLogLevel() const { return current_level; }
    void SetTimestampEnabled(bool enabled) { timestamp_enabled = enabled; }
    void SetComponentEnabled(const std::string& component, bool enabled);
    
    // Logging methods
    void Debug(const std::string& component, const char* format, ...);
    void Info(const std::string& component, const char* format, ...);
    void Warn(const std::string& component, const char* format, ...);
    void Error(const std::string& component, const char* format, ...);
    
    // Direct logging without component
    void Debug(const char* format, ...);
    void Info(const char* format, ...);
    void Warn(const char* format, ...);
    void Error(const char* format, ...);
    
    // Component-specific loggers
    static void LogDebug(const std::string& component, const char* format, ...);
    static void LogInfo(const std::string& component, const char* format, ...);
    static void LogWarn(const std::string& component, const char* format, ...);
    static void LogError(const std::string& component, const char* format, ...);

private:
    HaikuLogger() : current_level(LogLevel::INFO), timestamp_enabled(true) {}
    
    void Log(LogLevel level, const std::string& component, const char* format, va_list args);
    void Log(LogLevel level, const char* format, va_list args);
    
    std::string GetTimestamp() const;
    std::string GetLevelString(LogLevel level) const;
    bool ShouldLog(LogLevel level) const;
    
    LogLevel current_level;
    bool timestamp_enabled;
    std::map<std::string, bool> component_enabled;
    mutable std::mutex log_mutex;
    
    // Prevent copying
    HaikuLogger(const HaikuLogger&) = delete;
    HaikuLogger& operator=(const HaikuLogger&) = delete;
};

///////////////////////////////////////////////////////////////////////////
// Convenience Macros for Easy Logging
///////////////////////////////////////////////////////////////////////////

#define HAIKU_LOG_DEBUG(component, ...) HaikuLogger::LogDebug(component, __VA_ARGS__)
#define HAIKU_LOG_INFO(component, ...)  HaikuLogger::LogInfo(component, __VA_ARGS__)
#define HAIKU_LOG_WARN(component, ...)  HaikuLogger::LogWarn(component, __VA_ARGS__)
#define HAIKU_LOG_ERROR(component, ...) HaikuLogger::LogError(component, __VA_ARGS__)

// Component-specific macros
#define HAIKU_LOG_BEAPI(...)         HAIKU_LOG_INFO("BeAPI", __VA_ARGS__)
#define HAIKU_LOG_VM(...)            HAIKU_LOG_INFO("VM", __VA_ARGS__)
#define HAIKU_LOG_SYSCALL(...)       HAIKU_LOG_INFO("Syscall", __VA_ARGS__)
#define HAIKU_LOG_GUI(...)           HAIKU_LOG_INFO("GUI", __VA_ARGS__)
#define HAIKU_LOG_NETWORK(...)       HAIKU_LOG_INFO("Network", __VA_ARGS__)
#define HAIKU_LOG_KIT(...)           HAIKU_LOG_INFO("Kit", __VA_ARGS__)

// Warning-specific macros
#define HAIKU_LOG_BEAPI_WARN(...)    HAIKU_LOG_WARN("BeAPI", __VA_ARGS__)
#define HAIKU_LOG_VM_WARN(...)       HAIKU_LOG_WARN("VM", __VA_ARGS__)
#define HAIKU_LOG_SYSCALL_WARN(...)  HAIKU_LOG_WARN("Syscall", __VA_ARGS__)
#define HAIKU_LOG_GUI_WARN(...)      HAIKU_LOG_WARN("GUI", __VA_ARGS__)
#define HAIKU_LOG_NETWORK_WARN(...)  HAIKU_LOG_WARN("Network", __VA_ARGS__)
#define HAIKU_LOG_KIT_WARN(...)       HAIKU_LOG_WARN("Kit", __VA_ARGS__)

// Error-specific macros
#define HAIKU_LOG_BEAPI_ERROR(...)     HAIKU_LOG_ERROR("BeAPI", __VA_ARGS__)
#define HAIKU_LOG_VM_ERROR(...)        HAIKU_LOG_ERROR("VM", __VA_ARGS__)
#define HAIKU_LOG_SYSCALL_ERROR(...)   HAIKU_LOG_ERROR("Syscall", __VA_ARGS__)
#define HAIKU_LOG_GUI_ERROR(...)       HAIKU_LOG_ERROR("GUI", __VA_ARGS__)
#define HAIKU_LOG_NETWORK_ERROR(...)   HAIKU_LOG_ERROR("Network", __VA_ARGS__)
#define HAIKU_LOG_KIT_ERROR(...)       HAIKU_LOG_ERROR("Kit", __VA_ARGS__)

#endif // HAIKU_LOGGING_H