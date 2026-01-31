/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * PackageDownloader.cpp - Implementation of package downloads from HaikuDepot
 */

#include "PackageDownloader.h"
#include "DebugOutput.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// HaikuDepot API base URL
static const char* HAIKUDEPOT_BASE = "https://depot.haiku-os.org/__api/v1/packages";

PackageDownloader::PackageDownloader()
{
    fLastError[0] = '\0';
}

PackageDownloader::~PackageDownloader()
{
}

status_t PackageDownloader::Download(const char* url, const char* destination, bool show_progress)
{
    if (!url || !destination) {
        strcpy(fLastError, "Invalid URL or destination");
        return B_BAD_VALUE;
    }
    
    DebugPrintf("[PackageDownloader] Downloading from: %s\n", url);
    printf("[PackageDownloader] Downloading to: %s\n", destination);
    
    // For now, use system curl command (more portable than linking curl)
    // Production: could link libcurl for better control
    
    char command[1024];
    snprintf(command, sizeof(command), 
        "curl -L -f -o \"%s\" --progress-bar \"%s\" 2>&1",
        destination, url);
    
    DebugPrintf("[PackageDownloader] Executing: %s\n", command);
    
    int result = system(command);
    
    if (result != 0) {
        snprintf(fLastError, sizeof(fLastError), 
            "Download failed with exit code %d", result);
        DebugPrintf("[PackageDownloader] ERROR: %s\n", fLastError);
        return B_ERROR;
    }
    
    // Verify file exists
    if (!FileExists(destination)) {
        strcpy(fLastError, "Downloaded file not found");
        return B_FILE_NOT_FOUND;
    }
    
    printf("[PackageDownloader] ✓ Download complete\n");
    return B_OK;
}

const char* PackageDownloader::GetLastError()
{
    return fLastError;
}

bool PackageDownloader::FileExists(const char* path)
{
    if (!path) return false;
    return (access(path, R_OK) == 0);
}

status_t PackageDownloader::CalculateSHA256(const char* filepath, char* out_hash)
{
    if (!filepath || !out_hash) {
        return B_BAD_VALUE;
    }
    
    DebugPrintf("[PackageDownloader] Calculating SHA256 for: %s\n", filepath);
    
    // Use system sha256sum command
    char command[1024];
    snprintf(command, sizeof(command), "sha256sum \"%s\" 2>/dev/null | awk '{print $1}'", filepath);
    
    FILE* fp = popen(command, "r");
    if (!fp) {
        return B_ERROR;
    }
    
    // Read hash from command output
    if (fgets(out_hash, 65, fp) == NULL) {
        pclose(fp);
        return B_ERROR;
    }
    
    pclose(fp);
    
    // Remove newline
    size_t len = strlen(out_hash);
    if (len > 0 && out_hash[len-1] == '\n') {
        out_hash[len-1] = '\0';
    }
    
    DebugPrintf("[PackageDownloader] SHA256: %s\n", out_hash);
    return B_OK;
}

const char* PackageDownloader::BuildHaikuDepotURL(const char* package_name,
                                                  const char* version,
                                                  char* url_buffer)
{
    if (!package_name || !version || !url_buffer) {
        return NULL;
    }
    
    // Format: https://depot.haiku-os.org/__api/v1/packages/<package>/versions/<version>/download
    snprintf(url_buffer, 512, 
        "%s/%s/versions/%s/download",
        HAIKUDEPOT_BASE, package_name, version);
    
    DebugPrintf("[PackageDownloader] Built URL: %s\n", url_buffer);
    return url_buffer;
}

// CURL callback for progress (not used with system() but kept for future)
int PackageDownloader::ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                       curl_off_t ultotal, curl_off_t ulnow)
{
    if (dltotal > 0) {
        int percent = (int)((dlnow * 100) / dltotal);
        printf("\rDownload progress: %3d%%", percent);
        fflush(stdout);
    }
    return 0;
}
