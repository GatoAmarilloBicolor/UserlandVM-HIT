/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * SysrootManager.h - Manages sysroot setup and package downloads from HaikuDepot
 */

#pragma once

#include <OS.h>
#include <stdio.h>
#include <stdint.h>

/**
 * SysrootManager - Manages Haiku 32-bit system root
 * 
 * Responsibilities:
 * - Initialize sysroot directory structure
 * - Download packages from HaikuDepot
 * - Verify checksums
 * - Cache libraries locally
 * - Support background downloads
 */
class SysrootManager {
public:
    /**
     * Constructor
     * @param sysroot_path - Path to sysroot directory (e.g., "./sysroot/haiku32")
     */
    SysrootManager(const char* sysroot_path);
    ~SysrootManager();
    
    /**
     * Initialize sysroot directory structure
     * Creates necessary directories if they don't exist
     */
    status_t Initialize();
    
    /**
     * Ensure a library is available locally
     * Downloads if missing, returns path if exists
     * 
     * @param lib_name - Library name (e.g., "libc.so.0")
     * @param out_path - Output path buffer (at least 512 bytes)
     * @param async - If true, download in background (non-blocking)
     */
    status_t EnsureLibrary(const char* lib_name, char* out_path, bool async = false);
    
    /**
     * Download a package from HaikuDepot
     * Can run synchronously or asynchronously
     * 
     * @param package_url - Full URL to package
     * @param destination - Local destination path
     * @param async - If true, download in background thread
     */
    status_t DownloadPackage(const char* package_url, const char* destination, bool async);
    
    /**
     * Check if library exists locally
     */
    bool LibraryExists(const char* lib_name);
    
    /**
     * Get local path for library
     */
    const char* GetLibraryPath(const char* lib_name);
    
    /**
     * Wait for all background downloads to complete
     */
    status_t WaitForDownloads();
    
    /**
     * Get download progress (for UI)
     * Returns percentage (0-100)
     */
    int GetDownloadProgress();
    
    /**
     * Cancel all background downloads
     */
    status_t CancelDownloads();

private:
    char fSysrootPath[512];
    char fLibCachePath[512];
    char fHeadersPath[512];
    
    // Background download support
    thread_id fDownloadThread;
    volatile bool fDownloading;
    volatile int fDownloadProgress;
    
    /**
     * Static thread function for background downloads
     */
    static int32 DownloadThreadFunc(void* arg);
    
    /**
     * Internal: download implementation
     */
    status_t DownloadFile(const char* url, const char* destination);
    
    /**
     * Internal: verify checksum
     */
    bool VerifyChecksum(const char* file_path, const char* expected_hash);
    
    /**
     * Internal: get library URL from HaikuDepot
     */
    const char* GetLibraryURL(const char* lib_name);
};
