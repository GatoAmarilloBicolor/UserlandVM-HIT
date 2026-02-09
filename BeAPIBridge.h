/*
 * BeAPIBridge.h
 * 
 * UNIFIED Be API Bridge for ALL 32-bit Haiku applications
 * 
 * Any 32-bit app that calls Be API functions gets translated to 
 * real 64-bit Haiku Be API automatically.
 * 
 * Works for: WebPositive, Terminal, Tracker, Mail, Custom apps...
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <map>
#include <string>
#include <memory>

class BeAPIBridge {
public:
    static BeAPIBridge& GetInstance();
    
    // Initialize bridge to real Haiku Be API
    bool Initialize();
    
    // Shutdown
    void Shutdown();
    
    // Register guest app for Be API access
    bool RegisterApp(uint32_t app_id, const char* app_name);
    
    // Generic syscall handler for ALL Be API calls
    // This is the universal entry point for 32-bit apps
    uint32_t HandleBeAPISyscall(
        uint32_t app_id,      // Which 32-bit app is calling
        uint32_t syscall_num, // Syscall number
        uint32_t *args,       // Arguments (up to 6)
        uint32_t arg_count
    );
    
    // Window creation - works for any app
    uint32_t CreateWindow(uint32_t app_id, const char* title, 
                         uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    
    // Drawing - works for any window
    bool DrawLine(uint32_t app_id, uint32_t window_id, 
                 int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    bool FillRect(uint32_t app_id, uint32_t window_id,
                 int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    bool DrawString(uint32_t app_id, uint32_t window_id,
                   int32_t x, int32_t y, const char* text, uint32_t color);
    
    // Display update
    bool Flush(uint32_t app_id, uint32_t window_id);
    
    // Window management
    bool ShowWindow(uint32_t app_id, uint32_t window_id);
    bool HideWindow(uint32_t app_id, uint32_t window_id);
    bool DestroyWindow(uint32_t app_id, uint32_t window_id);
    
    // Check status
    bool IsConnected() const { return connected; }
    int GetWindowCount() const { return total_windows; }
    
private:
    BeAPIBridge();
    ~BeAPIBridge();
    
    // Singleton
    static BeAPIBridge* instance;
    
    // State
    bool connected;
    int total_windows;
    
    // Real Be API library handles
    void* libbe_handle;
    void* libinterface_handle;
    void* libapp_handle;
    
    // App registry: app_id -> app_name
    std::map<uint32_t, std::string> registered_apps;
    
    // Window registry: (app_id, window_id) -> real BWindow*
    std::map<std::pair<uint32_t, uint32_t>, void*> windows;
    
    // Load real Be API libraries
    bool LoadRealBeAPI();
    
    // Symbol mapping: Be API function name -> actual function pointer
    void* ResolveBeAPIFunction(const char* symbol_name);
    
    // Print status
    void PrintBeAPIStatus();
};
