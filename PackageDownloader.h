/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * PackageDownloader.h - Download packages from HaikuDepot with curl
 */

#pragma once

#include <OS.h>
#include <stdio.h>

/**
 * PackageDownloader - Downloads packages from HaikuDepot
 * Uses curl library for HTTP requests
 */
class PackageDownloader {
public:
    /**
     * Constructor
     */
    PackageDownloader();
    ~PackageDownloader();
    
    /**
     * Download file from URL
     * @param url - Full URL to download
     * @param destination - Where to save file
     * @param show_progress - Show download progress
     */
    status_t Download(const char* url, const char* destination, bool show_progress = true);
    
    /**
     * Get last error message
     */
    const char* GetLastError();
    
    /**
     * Verify file exists and is readable
     */
    static bool FileExists(const char* path);
    
    /**
     * Calculate SHA256 checksum of file
     * @param filepath - Path to file
     * @param out_hash - Output hash (65 bytes minimum for null-terminated hex string)
     */
    static status_t CalculateSHA256(const char* filepath, char* out_hash);
    
    /**
     * Build HaikuDepot URL for a package
     */
    static const char* BuildHaikuDepotURL(const char* package_name, 
                                         const char* version,
                                         char* url_buffer);

private:
    char fLastError[256];
    
    /**
     * CURL callback for progress
     */
    static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                               curl_off_t ultotal, curl_off_t ulnow);
};
