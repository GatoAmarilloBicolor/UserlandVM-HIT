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
#include "include/PlatformTypes.h"

class BeAPIBridge {
public:
    static BeAPIBridge& GetInstance();
    
    // Initialize bridge to real Haiku Be API
    bool Initialize();
    
    // Shutdown
    void Shutdown();
    
    // Register guest app for Be API access
    bool RegisterApp(haiku_id_t app_id, haiku_const_string_t app_name);
    
    // Generic syscall handler for ALL Be API calls
    // This is the universal entry point for 32-bit apps
    haiku_status_t HandleBeAPISyscall(
        haiku_id_t app_id,      // Which 32-bit app is calling
        haiku_id_t syscall_num, // Syscall number
        haiku_param_t *args,    // Arguments (up to 6)
        haiku_param_t arg_count
    );
    
    // Window creation - works for any app
    haiku_id_t CreateWindow(haiku_id_t app_id, haiku_const_string_t title, 
                           haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h);
    
    // Drawing - works for any window
    bool DrawLine(haiku_id_t app_id, haiku_id_t window_id, 
                 haiku_value_t x1, haiku_value_t y1, haiku_value_t x2, haiku_value_t y2, haiku_param_t color);
    bool FillRect(haiku_id_t app_id, haiku_id_t window_id,
                 haiku_value_t x, haiku_value_t y, haiku_value_t w, haiku_value_t h, haiku_param_t color);
    bool DrawString(haiku_id_t app_id, haiku_id_t window_id,
                   haiku_value_t x, haiku_value_t y, haiku_const_string_t text, haiku_param_t color);
    
    // Display update
    bool Flush(haiku_id_t app_id, haiku_id_t window_id);
    
    // Window management
    bool ShowWindow(haiku_id_t app_id, haiku_id_t window_id);
    bool HideWindow(haiku_id_t app_id, haiku_id_t window_id);
    bool DestroyWindow(haiku_id_t app_id, haiku_id_t window_id);
    
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
    std::map<haiku_id_t, std::string> registered_apps;
    
    // Window registry: (app_id, window_id) -> real BWindow*
    std::map<std::pair<haiku_id_t, haiku_id_t>, haiku_pointer_t> windows;
    
    // Load real Be API libraries
    bool LoadRealBeAPI();
    
    // Symbol mapping: Be API function name -> actual function pointer
    void* ResolveBeAPIFunction(const char* symbol_name);
    
    // Print status
    void PrintBeAPIStatus();
};
