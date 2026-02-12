/*
 * HaikuStorageKit.h - Complete Haiku Storage Kit Interface
 * 
 * Interface for all Haiku storage operations: BFile, BDirectory, BEntry, BPath, BVolume, BQuery
 */

#pragma once

#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <string>
#include <mutex>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Haiku storage constants
#define HAIKU_MAX_PATH_LENGTH           1024
#define HAIKU_MAX_FILE_DESCRIPTORS     256
#define HAIKU_MAX_DIRECTORY_DESCRIPTORS 64

// Haiku open modes (compatible with POSIX but Haiku-specific extensions)
#define HAIKU_O_RDONLY                 O_RDONLY
#define HAIKU_O_WRONLY                 O_WRONLY
#define HAIKU_O_RDWR                  O_RDWR
#define HAIKU_O_CREAT                 O_CREAT
#define HAIKU_O_EXCL                  O_EXCL
#define HAIKU_O_TRUNC                 O_TRUNC
#define HAIKU_O_APPEND                O_APPEND
#define HAIKU_O_NONBLOCK              O_NONBLOCK

// Haiku seek modes
#define HAIKU_SEEK_SET                SEEK_SET
#define HAIKU_SEEK_CUR                SEEK_CUR
#define HAIKU_SEEK_END                SEEK_END

// Haiku entry types
#define HAIKU_ENTRY_FILE              1
#define HAIKU_ENTRY_DIRECTORY         2
#define HAIKU_ENTRY_SYMLINK           3
#define HAIKU_ENTRY_UNKNOWN           0

// Haiku error codes for storage operations
#ifndef B_ENTRY_NOT_FOUND
#define B_ENTRY_NOT_FOUND            (-2147483634)
#endif
#ifndef B_FILE_EXISTS
#define B_FILE_EXISTS                (-2147483633)
#endif
#ifndef B_DIRECTORY_NOT_EMPTY
#define B_DIRECTORY_NOT_EMPTY        (-2147483632)
#endif
#ifndef B_NOT_SUPPORTED
#define B_NOT_SUPPORTED              (-2147483631)
#endif

// ============================================================================
// HAIKU STORAGE DATA STRUCTURES
// ============================================================================

/**
 * Haiku file descriptor information
 */
struct HaikuFileDescriptor {
    int fd;                          // POSIX file descriptor
    bool in_use;                      // Whether descriptor is in use
    uint32_t mode;                    // Open mode flags
    off_t position;                    // Current file position
    char path[HAIKU_MAX_PATH_LENGTH]; // File path
    
    HaikuFileDescriptor() : fd(-1), in_use(false), mode(0), position(0) {
        memset(path, 0, sizeof(path));
    }
};

/**
 * Haiku directory descriptor information
 */
struct HaikuDirectoryDescriptor {
    DIR* dir;                        // POSIX DIR pointer
    bool in_use;                      // Whether descriptor is in use
    char path[HAIKU_MAX_PATH_LENGTH]; // Directory path
    
    HaikuDirectoryDescriptor() : dir(nullptr), in_use(false) {
        memset(path, 0, sizeof(path));
    }
};

/**
 * Haiku entry information structure
 */
struct HaikuEntryInfo {
    uint64_t node;                    // Node ID (inode)
    uint32_t device;                  // Device ID
    off_t size;                       // Size in bytes
    time_t modified_time;              // Last modification time
    time_t created_time;               // Creation time
    uint32_t mode;                    // File mode/permissions
    uint32_t uid;                     // User ID
    uint32_t gid;                     // Group ID
    uint32_t type;                    // Entry type (file, directory, symlink)
    uint32_t padding;                 // Alignment padding
    
    HaikuEntryInfo() : node(0), device(0), size(0), modified_time(0),
                       created_time(0), mode(0), uid(0), gid(0), type(0),
                       padding(0) {}
};

// ============================================================================
// HAIKU STORAGE KIT INTERFACE
// ============================================================================

/**
 * Haiku Storage Kit implementation class
 * 
 * Provides complete Haiku storage functionality including file operations,
 * directory operations, entry management, and path utilities.
 */
class HaikuStorageKitImpl : public HaikuKit {
private:
    // File and directory descriptor tables
    HaikuFileDescriptor file_descriptors[HAIKU_MAX_FILE_DESCRIPTORS];
    HaikuDirectoryDescriptor directory_descriptors[HAIKU_MAX_DIRECTORY_DESCRIPTORS];
    
    // Next available descriptor IDs
    uint32_t next_file_id;
    uint32_t next_directory_id;
    
public:
    /**
     * Constructor
     */
    HaikuStorageKitImpl() : HaikuKit("Storage Kit") {
        memset(file_descriptors, 0, sizeof(file_descriptors));
        memset(directory_descriptors, 0, sizeof(directory_descriptors));
        next_file_id = 1;
        next_directory_id = 1;
    }
    
    /**
     * Destructor
     */
    virtual ~HaikuStorageKitImpl() {
        if (initialized) {
            Shutdown();
        }
    }
    
    // HaikuKit interface
    virtual status_t Initialize() ;
    virtual void Shutdown() ;
    
    /**
     * Get singleton instance
     */
    static HaikuStorageKitImpl& GetInstance();
    
    // ========================================================================
    // FILE OPERATIONS
    // ========================================================================
    
    /**
     * Open a file with specified mode
     * 
     * @param path File path
     * @param mode Open mode flags (O_RDONLY, O_WRONLY, O_CREAT, etc.)
     * @return File descriptor ID or 0 on error
     */
    virtual uint32_t OpenFile(const char* path, uint32_t mode);
    
    /**
     * Close an open file
     * 
     * @param file_id File descriptor ID
     * @return Status code
     */
    virtual status_t CloseFile(uint32_t file_id);
    
    /**
     * Read data from file
     * 
     * @param file_id File descriptor ID
     * @param buffer Buffer to read into
     * @param size Number of bytes to read
     * @return Number of bytes read or -1 on error
     */
    virtual ssize_t ReadFile(uint32_t file_id, void* buffer, size_t size) ;
    
    /**
     * Write data to file
     * 
     * @param file_id File descriptor ID
     * @param buffer Buffer to write from
     * @param size Number of bytes to write
     * @return Number of bytes written or -1 on error
     */
    virtual ssize_t WriteFile(uint32_t file_id, const void* buffer, size_t size) ;
    
    /**
     * Seek to position in file
     * 
     * @param file_id File descriptor ID
     * @param position Position to seek to
     * @param seek_mode Seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
     * @return Status code
     */
    virtual status_t SeekFile(uint32_t file_id, off_t position, uint32_t seek_mode) ;
    
    /**
     * Set file size
     * 
     * @param file_id File descriptor ID
     * @param size New file size
     * @return Status code
     */
    virtual status_t SetFileSize(uint32_t file_id, off_t size) ;
    
    // ========================================================================
    // DIRECTORY OPERATIONS
    // ========================================================================
    
    /**
     * Open a directory
     * 
     * @param path Directory path
     * @return Directory descriptor ID or 0 on error
     */
    virtual uint32_t OpenDirectory(const char* path) ;
    
    /**
     * Close an open directory
     * 
     * @param dir_id Directory descriptor ID
     * @return Status code
     */
    virtual status_t CloseDirectory(uint32_t dir_id) ;
    
    /**
     * Read next entry from directory
     * 
     * @param dir_id Directory descriptor ID
     * @param name Buffer to store entry name
     * @param size Size of buffer
     * @return Status code (B_ENTRY_NOT_FOUND when end of directory)
     */
    virtual status_t ReadDirectory(uint32_t dir_id, char* name, size_t size) ;
    
    /**
     * Rewind directory to beginning
     * 
     * @param dir_id Directory descriptor ID
     * @return Status code
     */
    virtual status_t RewindDirectory(uint32_t dir_id) ;
    
    // ========================================================================
    // ENTRY OPERATIONS
    // ========================================================================
    
    /**
     * Get entry information
     * 
     * @param path Entry path
     * @param info Buffer to store entry info
     * @return Status code
     */
    virtual status_t GetEntryInfo(const char* path, void* info) ;
    
    /**
     * Create a new entry
     * 
     * @param path Path for new entry
     * @param type Entry type (HAIKU_ENTRY_FILE, HAIKU_ENTRY_DIRECTORY, etc.)
     * @return Status code
     */
    virtual status_t CreateEntry(const char* path, uint32_t type) ;
    
    /**
     * Delete an entry
     * 
     * @param path Entry path
     * @return Status code
     */
    virtual status_t DeleteEntry(const char* path) ;
    
    /**
     * Move/rename an entry
     * 
     * @param old_path Current path
     * @param new_path New path
     * @return Status code
     */
    virtual status_t MoveEntry(const char* old_path, const char* new_path) ;
    
    // ========================================================================
    // PATH OPERATIONS
    // ========================================================================
    
    /**
     * Get absolute path
     * 
     * @param path Relative or absolute path
     * @param abs_path Buffer to store absolute path
     * @param size Size of buffer
     * @return Status code
     */
    virtual status_t GetAbsolutePath(const char* path, char* abs_path, size_t size) ;
    
    /**
     * Get parent directory path
     * 
     * @param path Path
     * @param parent_path Buffer to store parent path
     * @param size Size of buffer
     * @return Status code
     */
    virtual status_t GetParentPath(const char* path, char* parent_path, size_t size) ;
    
    // ========================================================================
    // UTILITY METHODS
    // ========================================================================
    
    /**
     * Get file descriptor information
     */
    const HaikuFileDescriptor* GetFileInfo(uint32_t file_id) const {
        if (file_id > 0 && file_id < HAIKU_MAX_FILE_DESCRIPTORS) {
            return &file_descriptors[file_id];
        }
        return nullptr;
    }
    
    /**
     * Get directory descriptor information
     */
    const HaikuDirectoryDescriptor* GetDirectoryInfo(uint32_t dir_id) const {
        if (dir_id > 0 && dir_id < HAIKU_MAX_DIRECTORY_DESCRIPTORS) {
            return &directory_descriptors[dir_id];
        }
        return nullptr;
    }
    
    /**
     * Get storage statistics
     */
    void GetStorageStats(uint32_t* open_files, uint32_t* open_directories) {
        uint32_t files = 0, dirs = 0;
        
        for (uint32_t i = 1; i < HAIKU_MAX_FILE_DESCRIPTORS; i++) {
            if (file_descriptors[i].in_use) files++;
        }
        
        for (uint32_t i = 1; i < HAIKU_MAX_DIRECTORY_DESCRIPTORS; i++) {
            if (directory_descriptors[i].in_use) dirs++;
        }
        
        if (open_files) *open_files = files;
        if (open_directories) *open_directories = dirs;
    }
    
    /**
     * Dump storage state for debugging
     */
    void DumpStorageState() const {
        printf("[HAIKU_STORAGE] Storage State Dump:\n");
        printf("  Open files: ");
        
        for (uint32_t i = 1; i < HAIKU_MAX_FILE_DESCRIPTORS; i++) {
            if (file_descriptors[i].in_use) {
                printf("%u(%s) ", i, file_descriptors[i].path);
            }
        }
        printf("\n");
        
        printf("  Open directories: ");
        for (uint32_t i = 1; i < HAIKU_MAX_DIRECTORY_DESCRIPTORS; i++) {
            if (directory_descriptors[i].in_use) {
                printf("%u(%s) ", i, directory_descriptors[i].path);
            }
        }
        printf("\n");
    }
};