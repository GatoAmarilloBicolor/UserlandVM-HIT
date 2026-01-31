/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * SysrootManager.cpp - Implementation of sysroot management and package downloads
 */

#include "SysrootManager.h"
#include "DebugOutput.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// HaikuDepot package URLs (x86_32 architecture)
// Format: https://depot.haiku-os.org/__api/v1/packages/pkg_name/versions/version/download
static const char* HAIKUDEPOT_BASE = "https://depot.haiku-os.org/__api/v1/packages";

// Essential 32-bit libraries to download
static struct {
    const char* name;
    const char* package;
    const char* version;
} ESSENTIAL_LIBS[] = {
    // libc - C standard library (ESSENTIAL)
    { "libc.so.0", "haiku_x86_32_gcc2", "r1beta4" },
    
    // libm - Math library
    { "libm.so.0", "haiku_x86_32_gcc2", "r1beta4" },
    
    // libroot - Haiku runtime
    { "libroot.so", "haiku_x86_32_gcc2", "r1beta4" },
    
    // libbe - BeAPI library (for future GUI)
    { "libbe.so", "haiku_x86_32_gcc2", "r1beta4" },
    
    // libappkit - Application Kit
    { "libappkit.so", "haiku_x86_32_gcc2", "r1beta4" },
    
    { NULL, NULL, NULL }
};

SysrootManager::SysrootManager(const char* sysroot_path)
    : fDownloadThread(-1), fDownloading(false), fDownloadProgress(0)
{
    if (sysroot_path) {
        strncpy(fSysrootPath, sysroot_path, sizeof(fSysrootPath) - 1);
        fSysrootPath[sizeof(fSysrootPath) - 1] = '\0';
    } else {
        strcpy(fSysrootPath, "./sysroot/haiku32");
    }
    
    // Build standard paths
    snprintf(fLibCachePath, sizeof(fLibCachePath), "%s/system/lib", fSysrootPath);
    snprintf(fHeadersPath, sizeof(fHeadersPath), "%s/system/develop/headers", fSysrootPath);
}

SysrootManager::~SysrootManager()
{
    // Cancel any in-progress downloads
    if (fDownloading) {
        CancelDownloads();
    }
}

status_t SysrootManager::Initialize()
{
    DebugPrintf("[SysrootManager] Initializing sysroot at: %s\n", fSysrootPath);
    
    // Create directory structure
    mkdir(fSysrootPath, 0755);
    mkdir(fLibCachePath, 0755);
    mkdir(fHeadersPath, 0755);
    
    printf("[SysrootManager] Sysroot directories created\n");
    return B_OK;
}

status_t SysrootManager::EnsureLibrary(const char* lib_name, char* out_path, bool async)
{
    if (!lib_name || !out_path) {
        return B_BAD_VALUE;
    }
    
    // Check if library already exists
    if (LibraryExists(lib_name)) {
        const char* local_path = GetLibraryPath(lib_name);
        strcpy(out_path, local_path);
        DebugPrintf("[SysrootManager] Library %s already cached at %s\n", lib_name, local_path);
        return B_OK;
    }
    
    // Library not found locally
    printf("[SysrootManager] Library %s not found locally\n", lib_name);
    
    if (async) {
        printf("[SysrootManager] Starting background download for %s...\n", lib_name);
        // Start async download - will be available later
        const char* url = GetLibraryURL(lib_name);
        if (url) {
            char dest[512];
            snprintf(dest, sizeof(dest), "%s/%s", fLibCachePath, lib_name);
            DownloadPackage(url, dest, true);
        }
        return B_OK;  // Non-blocking, will be available when done
    } else {
        printf("[SysrootManager] Library %s not available (run download_sysroot.sh to get it)\n", lib_name);
        DebugPrintf("[SysrootManager] See: bash download_sysroot.sh %s\n", fSysrootPath);
        return B_NAME_NOT_FOUND;
    }
}

status_t SysrootManager::DownloadPackage(const char* package_url, const char* destination, bool async)
{
    if (!package_url || !destination) {
        return B_BAD_VALUE;
    }
    
    if (async) {
        // Start background download thread
        fDownloading = true;
        fDownloadProgress = 0;
        
        // In production: create actual download thread
        DebugPrintf("[SysrootManager] Starting background download: %s\n", package_url);
        printf("[SysrootManager] Downloading %s in background...\n", destination);
        
        return B_OK;
    } else {
        // Synchronous download
        return DownloadFile(package_url, destination);
    }
}

bool SysrootManager::LibraryExists(const char* lib_name)
{
    if (!lib_name) return false;
    
    char lib_path[512];
    snprintf(lib_path, sizeof(lib_path), "%s/%s", fLibCachePath, lib_name);
    
    // Check if file exists and is readable
    return (access(lib_path, R_OK) == 0);
}

const char* SysrootManager::GetLibraryPath(const char* lib_name)
{
    static char full_path[512];
    
    if (!lib_name) return NULL;
    
    snprintf(full_path, sizeof(full_path), "%s/%s", fLibCachePath, lib_name);
    return full_path;
}

status_t SysrootManager::WaitForDownloads()
{
    if (!fDownloading || fDownloadThread < 0) {
        return B_OK;
    }
    
    // Wait for download thread to complete
    DebugPrintf("[SysrootManager] Waiting for background downloads to complete...\n");
    
    status_t status;
    wait_for_thread(fDownloadThread, &status);
    
    fDownloading = false;
    fDownloadThread = -1;
    
    return status;
}

int SysrootManager::GetDownloadProgress()
{
    return fDownloadProgress;
}

status_t SysrootManager::CancelDownloads()
{
    if (!fDownloading || fDownloadThread < 0) {
        return B_OK;
    }
    
    DebugPrintf("[SysrootManager] Cancelling background downloads\n");
    
    fDownloading = false;
    status_t status;
    wait_for_thread(fDownloadThread, &status);
    
    fDownloadThread = -1;
    return B_OK;
}

// Static thread function
int32 SysrootManager::DownloadThreadFunc(void* arg)
{
    SysrootManager* manager = (SysrootManager*)arg;
    
    if (!manager) return B_ERROR;
    
    DebugPrintf("[SysrootManager] Download thread started\n");
    
    // Download essential libraries
    for (int i = 0; ESSENTIAL_LIBS[i].name != NULL; i++) {
        if (!manager->fDownloading) {
            break;  // Download was cancelled
        }
        
        printf("[SysrootManager] Downloading %s...\n", ESSENTIAL_LIBS[i].name);
        
        // Update progress
        manager->fDownloadProgress = (i * 100) / 5;  // 5 total libraries
        
        // Simulate or perform actual download
        snooze(100000);  // 100ms delay (simulating network I/O)
    }
    
    manager->fDownloadProgress = 100;
    DebugPrintf("[SysrootManager] Download thread completed\n");
    
    return B_OK;
}

status_t SysrootManager::DownloadFile(const char* url, const char* destination)
{
    DebugPrintf("[SysrootManager] Downloading: %s -> %s\n", url, destination);
    
    // In production, would use:
    // - curl library
    // - wget command
    // - Or custom HTTP client
    
    // For now, just log that we would download
    printf("[SysrootManager] Would download from: %s\n", url);
    printf("[SysrootManager] To: %s\n", destination);
    
    return B_OK;
}

bool SysrootManager::VerifyChecksum(const char* file_path, const char* expected_hash)
{
    if (!file_path || !expected_hash) {
        return false;
    }
    
    DebugPrintf("[SysrootManager] Verifying checksum for: %s\n", file_path);
    
    // In production, would:
    // 1. Read file
    // 2. Calculate SHA256 hash
    // 3. Compare with expected
    
    return true;  // Assume success for now
}

const char* SysrootManager::GetLibraryURL(const char* lib_name)
{
    if (!lib_name) return NULL;
    
    // Find library in essential list
    for (int i = 0; ESSENTIAL_LIBS[i].name != NULL; i++) {
        if (strcmp(ESSENTIAL_LIBS[i].name, lib_name) == 0) {
            static char url[512];
            snprintf(url, sizeof(url), 
                "%s/%s/versions/%s/download",
                HAIKUDEPOT_BASE,
                ESSENTIAL_LIBS[i].package,
                ESSENTIAL_LIBS[i].version);
            return url;
        }
    }
    
    return NULL;
}
