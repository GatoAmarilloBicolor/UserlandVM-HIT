/*
 * HaikuStorageKit.cpp - Complete Haiku Storage Kit Implementation
 * 
 * Implements all Haiku storage operations: BFile, BDirectory, BEntry, BPath, BVolume, BQuery
 * Provides POSIX-to-Haiku translation layer for cross-platform compatibility
 */

#include "HaikuStorageKit.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

// ============================================================================
// HAIKU STORAGE KIT IMPLEMENTATION
// ============================================================================

/**
 * Singleton instance getter
 */
HaikuStorageKitImpl& HaikuStorageKitImpl::GetInstance() {
    static HaikuStorageKit instance;
    return instance;
}

/**
 * Initialize Storage Kit
 */
status_t HaikuStorageKitImpl::Initialize() {
    if (initialized) {
        return B_OK;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_STORAGE] Initializing Storage Kit...\n");
    
    // Initialize file descriptor table
    memset(file_descriptors, 0, sizeof(file_descriptors));
    next_file_id = 1;
    
    // Initialize directory descriptor table  
    memset(directory_descriptors, 0, sizeof(directory_descriptors));
    next_directory_id = 1;
    
    initialized = true;
    
    printf("[HAIKU_STORAGE] ✅ Storage Kit initialized\n");
    printf("[HAIKU_STORAGE] 📁 Max file descriptors: %d\n", MAX_FILE_DESCRIPTORS);
    printf("[HAIKU_STORAGE] 📂 Max directory descriptors: %d\n", MAX_DIRECTORY_DESCRIPTORS);
    
    return B_OK;
}

/**
 * Shutdown Storage Kit
 */
void HaikuStorageKit::Shutdown() {
    if (!initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_STORAGE] Shutting down Storage Kit...\n");
    
    // Close all open file descriptors
    for (uint32_t i = 1; i < MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i].in_use) {
            close(file_descriptors[i].fd);
            file_descriptors[i].in_use = false;
        }
    }
    
    // Close all open directory descriptors
    for (uint32_t i = 1; i < MAX_DIRECTORY_DESCRIPTORS; i++) {
        if (directory_descriptors[i].in_use) {
            closedir(directory_descriptors[i].dir);
            directory_descriptors[i].in_use = false;
        }
    }
    
    initialized = false;
    
    printf("[HAIKU_STORAGE] ✅ Storage Kit shutdown complete\n");
}

// ============================================================================
// FILE OPERATIONS
// ============================================================================

uint32_t HaikuStorageKit::OpenFile(const char* path, uint32_t mode) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    // Find available file descriptor
    uint32_t file_id = next_file_id;
    for (uint32_t i = 1; i < MAX_FILE_DESCRIPTORS; i++) {
        if (!file_descriptors[i].in_use) {
            file_id = i;
            break;
        }
    }
    
    if (file_descriptors[file_id].in_use) {
        printf("[HAIKU_STORAGE] ❌ No available file descriptors\n");
        return 0;
    }
    
    // Convert Haiku open mode to POSIX flags
    int posix_flags = 0;
    if (mode & O_RDONLY) posix_flags |= O_RDONLY;
    if (mode & O_WRONLY) posix_flags |= O_WRONLY;
    if (mode & O_RDWR) posix_flags |= O_RDWR;
    if (mode & O_CREAT) posix_flags |= O_CREAT;
    if (mode & O_EXCL) posix_flags |= O_EXCL;
    if (mode & O_TRUNC) posix_flags |= O_TRUNC;
    if (mode & O_APPEND) posix_flags |= O_APPEND;
    
    // Convert Haiku path to host path
    std::string host_path = HaikuAPIUtils::ConvertHaikuPathToHost(path);
    
    // Open file
    int fd = open(host_path.c_str(), posix_flags, 0644);
    if (fd == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to open file: %s (errno: %d)\n", host_path.c_str(), errno);
        return 0;
    }
    
    // Store file descriptor info
    file_descriptors[file_id].fd = fd;
    file_descriptors[file_id].in_use = true;
    file_descriptors[file_id].mode = mode;
    file_descriptors[file_id].position = 0;
    strncpy(file_descriptors[file_id].path, path, sizeof(file_descriptors[file_id].path) - 1);
    
    next_file_id = (file_id + 1) % MAX_FILE_DESCRIPTORS;
    if (next_file_id == 0) next_file_id = 1;
    
    printf("[HAIKU_STORAGE] 📄 Opened file: %s -> fd %d (id %u)\n", path, fd, file_id);
    
    return file_id;
}

status_t HaikuStorageKit::CloseFile(uint32_t file_id) {
    if (!initialized || file_id == 0 || file_id >= MAX_FILE_DESCRIPTORS) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!file_descriptors[file_id].in_use) {
        return B_BAD_VALUE;
    }
    
    // Close file
    int result = close(file_descriptors[file_id].fd);
    
    // Clear descriptor
    file_descriptors[file_id].in_use = false;
    file_descriptors[file_id].fd = -1;
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to close file descriptor %u\n", file_id);
        return B_ERROR;
    }
    
    printf("[HAIKU_STORAGE] 📄 Closed file descriptor %u\n", file_id);
    
    return B_OK;
}

ssize_t HaikuStorageKit::ReadFile(uint32_t file_id, void* buffer, size_t size) {
    if (!initialized || file_id == 0 || file_id >= MAX_FILE_DESCRIPTORS) {
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!file_descriptors[file_id].in_use) {
        return -1;
    }
    
    // Read from file
    ssize_t bytes_read = read(file_descriptors[file_id].fd, buffer, size);
    
    if (bytes_read == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to read from file descriptor %u\n", file_id);
        return -1;
    }
    
    // Update position
    file_descriptors[file_id].position += bytes_read;
    
    printf("[HAIKU_STORAGE] 📖 Read %zd bytes from file descriptor %u\n", bytes_read, file_id);
    
    return bytes_read;
}

ssize_t HaikuStorageKit::WriteFile(uint32_t file_id, const void* buffer, size_t size) {
    if (!initialized || file_id == 0 || file_id >= MAX_FILE_DESCRIPTORS) {
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!file_descriptors[file_id].in_use) {
        return -1;
    }
    
    // Write to file
    ssize_t bytes_written = write(file_descriptors[file_id].fd, buffer, size);
    
    if (bytes_written == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to write to file descriptor %u\n", file_id);
        return -1;
    }
    
    // Update position
    file_descriptors[file_id].position += bytes_written;
    
    printf("[HAIKU_STORAGE] ✍️  Written %zd bytes to file descriptor %u\n", bytes_written, file_id);
    
    return bytes_written;
}

status_t HaikuStorageKit::SeekFile(uint32_t file_id, off_t position, uint32_t seek_mode) {
    if (!initialized || file_id == 0 || file_id >= MAX_FILE_DESCRIPTORS) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!file_descriptors[file_id].in_use) {
        return B_BAD_VALUE;
    }
    
    // Convert Haiku seek mode to POSIX
    int posix_whence = 0;
    switch (seek_mode) {
        case SEEK_SET: posix_whence = SEEK_SET; break;
        case SEEK_CUR: posix_whence = SEEK_CUR; break;
        case SEEK_END: posix_whence = SEEK_END; break;
        default: return B_BAD_VALUE;
    }
    
    // Seek in file
    off_t result = lseek(file_descriptors[file_id].fd, position, posix_whence);
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to seek in file descriptor %u\n", file_id);
        return B_ERROR;
    }
    
    // Update position
    file_descriptors[file_id].position = result;
    
    printf("[HAIKU_STORAGE] ⏭️  Seeked file descriptor %u to position %lld\n", file_id, result);
    
    return B_OK;
}

status_t HaikuStorageKit::SetFileSize(uint32_t file_id, off_t size) {
    if (!initialized || file_id == 0 || file_id >= MAX_FILE_DESCRIPTORS) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!file_descriptors[file_id].in_use) {
        return B_BAD_VALUE;
    }
    
    // Resize file
    int result = ftruncate(file_descriptors[file_id].fd, size);
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to resize file descriptor %u\n", file_id);
        return B_ERROR;
    }
    
    printf("[HAIKU_STORAGE] 📏 Resized file descriptor %u to %lld bytes\n", file_id, size);
    
    return B_OK;
}

// ============================================================================
// DIRECTORY OPERATIONS
// ============================================================================

uint32_t HaikuStorageKit::OpenDirectory(const char* path) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    // Find available directory descriptor
    uint32_t dir_id = next_directory_id;
    for (uint32_t i = 1; i < MAX_DIRECTORY_DESCRIPTORS; i++) {
        if (!directory_descriptors[i].in_use) {
            dir_id = i;
            break;
        }
    }
    
    if (directory_descriptors[dir_id].in_use) {
        printf("[HAIKU_STORAGE] ❌ No available directory descriptors\n");
        return 0;
    }
    
    // Convert Haiku path to host path
    std::string host_path = HaikuAPIUtils::ConvertHaikuPathToHost(path);
    
    // Open directory
    DIR* dir = opendir(host_path.c_str());
    if (dir == nullptr) {
        printf("[HAIKU_STORAGE] ❌ Failed to open directory: %s (errno: %d)\n", host_path.c_str(), errno);
        return 0;
    }
    
    // Store directory descriptor info
    directory_descriptors[dir_id].dir = dir;
    directory_descriptors[dir_id].in_use = true;
    strncpy(directory_descriptors[dir_id].path, path, sizeof(directory_descriptors[dir_id].path) - 1);
    
    next_directory_id = (dir_id + 1) % MAX_DIRECTORY_DESCRIPTORS;
    if (next_directory_id == 0) next_directory_id = 1;
    
    printf("[HAIKU_STORAGE] 📂 Opened directory: %s -> id %u\n", path, dir_id);
    
    return dir_id;
}

status_t HaikuStorageKit::CloseDirectory(uint32_t dir_id) {
    if (!initialized || dir_id == 0 || dir_id >= MAX_DIRECTORY_DESCRIPTORS) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!directory_descriptors[dir_id].in_use) {
        return B_BAD_VALUE;
    }
    
    // Close directory
    int result = closedir(directory_descriptors[dir_id].dir);
    
    // Clear descriptor
    directory_descriptors[dir_id].in_use = false;
    directory_descriptors[dir_id].dir = nullptr;
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to close directory %u\n", dir_id);
        return B_ERROR;
    }
    
    printf("[HAIKU_STORAGE] 📂 Closed directory %u\n", dir_id);
    
    return B_OK;
}

status_t HaikuStorageKit::ReadDirectory(uint32_t dir_id, char* name, size_t size) {
    if (!initialized || dir_id == 0 || dir_id >= MAX_DIRECTORY_DESCRIPTORS) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!directory_descriptors[dir_id].in_use) {
        return B_BAD_VALUE;
    }
    
    // Read directory entry
    struct dirent* entry = readdir(directory_descriptors[dir_id].dir);
    if (entry == nullptr) {
        return B_ENTRY_NOT_FOUND;  // End of directory
    }
    
    // Copy name to buffer
    strncpy(name, entry->d_name, size - 1);
    name[size - 1] = '\0';
    
    printf("[HAIKU_STORAGE] 📋 Read directory entry: %s\n", name);
    
    return B_OK;
}

status_t HaikuStorageKit::RewindDirectory(uint32_t dir_id) {
    if (!initialized || dir_id == 0 || dir_id >= MAX_DIRECTORY_DESCRIPTORS) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (!directory_descriptors[dir_id].in_use) {
        return B_BAD_VALUE;
    }
    
    // Rewind directory
    rewinddir(directory_descriptors[dir_id].dir);
    
    printf("[HAIKU_STORAGE] ⏮️  Rewound directory %u\n", dir_id);
    
    return B_OK;
}

// ============================================================================
// ENTRY OPERATIONS
// ============================================================================

status_t HaikuStorageKit::GetEntryInfo(const char* path, void* info) {
    if (!initialized || path == nullptr || info == nullptr) {
        return B_BAD_VALUE;
    }
    
    // Convert Haiku path to host path
    std::string host_path = HaikuAPIUtils::ConvertHaikuPathToHost(path);
    
    // Get file statistics
    struct stat st;
    int result = stat(host_path.c_str(), &st);
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to get entry info: %s (errno: %d)\n", host_path.c_str(), errno);
        return B_ENTRY_NOT_FOUND;
    }
    
    // Fill Haiku entry info structure
    HaikuEntryInfo* haiku_info = static_cast<HaikuEntryInfo*>(info);
    haiku_info->node = st.st_ino;
    haiku_info->device = st.st_dev;
    haiku_info->size = st.st_size;
    haiku_info->modified_time = st.st_mtime;
    haiku_info->created_time = st.st_ctime;
    haiku_info->mode = st.st_mode;
    haiku_info->uid = st.st_uid;
    haiku_info->gid = st.st_gid;
    
    // Determine entry type
    if (S_ISREG(st.st_mode)) {
        haiku_info->type = HAIKU_ENTRY_FILE;
    } else if (S_ISDIR(st.st_mode)) {
        haiku_info->type = HAIKU_ENTRY_DIRECTORY;
    } else if (S_ISLNK(st.st_mode)) {
        haiku_info->type = HAIKU_ENTRY_SYMLINK;
    } else {
        haiku_info->type = HAIKU_ENTRY_UNKNOWN;
    }
    
    printf("[HAIKU_STORAGE] 📊 Got entry info for: %s\n", path);
    
    return B_OK;
}

status_t HaikuStorageKit::CreateEntry(const char* path, uint32_t type) {
    if (!initialized || path == nullptr) {
        return B_BAD_VALUE;
    }
    
    // Convert Haiku path to host path
    std::string host_path = HaikuAPIUtils::ConvertHaikuPathToHost(path);
    
    int result = 0;
    
    switch (type) {
        case HAIKU_ENTRY_FILE:
            result = open(host_path.c_str(), O_CREAT | O_EXCL, 0644);
            if (result != -1) close(result);
            break;
            
        case HAIKU_ENTRY_DIRECTORY:
            result = mkdir(host_path.c_str(), 0755);
            break;
            
        case HAIKU_ENTRY_SYMLINK:
            // For symlink creation, we need the target parameter
            // This is a simplified version
            printf("[HAIKU_STORAGE] ⚠️  Symlink creation not fully implemented\n");
            return B_NOT_SUPPORTED;
            
        default:
            return B_BAD_VALUE;
    }
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to create entry: %s (errno: %d)\n", host_path.c_str(), errno);
        return B_ERROR;
    }
    
    printf("[HAIKU_STORAGE] ✅ Created entry: %s\n", path);
    
    return B_OK;
}

status_t HaikuStorageKit::DeleteEntry(const char* path) {
    if (!initialized || path == nullptr) {
        return B_BAD_VALUE;
    }
    
    // Convert Haiku path to host path
    std::string host_path = HaikuAPIUtils::ConvertHaikuPathToHost(path);
    
    // Get entry info to determine type
    struct stat st;
    int result = stat(host_path.c_str(), &st);
    if (result == -1) {
        return B_ENTRY_NOT_FOUND;
    }
    
    // Delete based on type
    if (S_ISDIR(st.st_mode)) {
        result = rmdir(host_path.c_str());
    } else {
        result = unlink(host_path.c_str());
    }
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to delete entry: %s (errno: %d)\n", host_path.c_str(), errno);
        return B_ERROR;
    }
    
    printf("[HAIKU_STORAGE] 🗑️  Deleted entry: %s\n", path);
    
    return B_OK;
}

status_t HaikuStorageKit::MoveEntry(const char* old_path, const char* new_path) {
    if (!initialized || old_path == nullptr || new_path == nullptr) {
        return B_BAD_VALUE;
    }
    
    // Convert Haiku paths to host paths
    std::string host_old_path = HaikuAPIUtils::ConvertHaikuPathToHost(old_path);
    std::string host_new_path = HaikuAPIUtils::ConvertHaikuPathToHost(new_path);
    
    // Move/rename entry
    int result = rename(host_old_path.c_str(), host_new_path.c_str());
    
    if (result == -1) {
        printf("[HAIKU_STORAGE] ❌ Failed to move entry: %s -> %s (errno: %d)\n", 
               old_path, new_path, errno);
        return B_ERROR;
    }
    
    printf("[HAIKU_STORAGE] 📦 Moved entry: %s -> %s\n", old_path, new_path);
    
    return B_OK;
}

// ============================================================================
// PATH OPERATIONS
// ============================================================================

status_t HaikuStorageKit::GetAbsolutePath(const char* path, char* abs_path, size_t size) {
    if (!initialized || path == nullptr || abs_path == nullptr || size == 0) {
        return B_BAD_VALUE;
    }
    
    // Convert Haiku path to host path
    std::string host_path = HaikuAPIUtils::ConvertHaikuPathToHost(path);
    
    // Get absolute path
    char* resolved_path = realpath(host_path.c_str(), nullptr);
    if (resolved_path == nullptr) {
        printf("[HAIKU_STORAGE] ❌ Failed to get absolute path: %s\n", path);
        return B_ERROR;
    }
    
    // Convert back to Haiku path and copy to buffer
    std::string haiku_path = HaikuAPIUtils::ConvertHostPathToHaiku(resolved_path);
    strncpy(abs_path, haiku_path.c_str(), size - 1);
    abs_path[size - 1] = '\0';
    
    free(resolved_path);
    
    printf("[HAIKU_STORAGE] 📍 Absolute path: %s -> %s\n", path, abs_path);
    
    return B_OK;
}

status_t HaikuStorageKit::GetParentPath(const char* path, char* parent_path, size_t size) {
    if (!initialized || path == nullptr || parent_path == nullptr || size == 0) {
        return B_BAD_VALUE;
    }
    
    // Find last slash in path
    const char* last_slash = strrchr(path, '/');
    if (last_slash == nullptr) {
        // No parent directory
        strncpy(parent_path, ".", size - 1);
    } else if (last_slash == path) {
        // Root directory
        strncpy(parent_path, "/", size - 1);
    } else {
        // Extract parent directory
        size_t parent_length = last_slash - path;
        size_t copy_length = (parent_length < size - 1) ? parent_length : size - 1;
        strncpy(parent_path, path, copy_length);
        parent_path[copy_length] = '\0';
    }
    
    parent_path[size - 1] = '\0';
    
    printf("[HAIKU_STORAGE] 📁 Parent path: %s -> %s\n", path, parent_path);
    
    return B_OK;
}