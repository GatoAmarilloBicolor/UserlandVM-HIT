/*
 * HaikuSupportKit.h - Complete Haiku Support Kit Interface
 * 
 * Interface for all Haiku support utilities: BString, BList, BObjectList, BLocker, BPoint, BRect
 * Provides cross-platform Haiku support functionality
 */

#pragma once

#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

// Haiku Support Kit constants
#define HAIKU_MAX_STRINGS           2048
#define HAIKU_MAX_LISTS             1024
#define HAIKU_MAX_LOCKERS           256
#define HAIKU_MAX_STRING_LENGTH     8192
#define HAIKU_MAX_LIST_ITEMS         65536
#define HAIKU_MAX_OBJECTS           32768

// Haiku string constants
#define HAIKU_STRING_GROWTH_FACTOR  2
#define HAIKU_STRING_INITIAL_SIZE   64

// ============================================================================
// HAIKU SUPPORT KIT DATA STRUCTURES
// ============================================================================

/**
 * Haiku string information
 */
struct HaikuString {
    char* data;
    size_t length;
    size_t capacity;
    uint32_t id;
    
    HaikuString() : data(nullptr), length(0), capacity(0), id(0) {}
    
    ~HaikuString() {
        if (data) {
            delete[] data;
        }
    }
};

/**
 * Haiku list item structure
 */
struct HaikuListItem {
    void* data;
    bool owns_data;
    uint32_t id;
    
    HaikuListItem() : data(nullptr), owns_data(false), id(0) {}
    
    ~HaikuListItem() {
        if (owns_data && data) {
            free(data);
        }
    }
};

/**
 * Haiku list information
 */
struct HaikuList {
    std::vector<HaikuListItem*> items;
    bool owns_items;
    bool item_ownership;
    uint32_t id;
    
    HaikuList() : owns_items(true), item_ownership(true), id(0) {}
    
    ~HaikuList() {
        Clear();
    }
    
    void Clear() {
        if (owns_items) {
            for (auto item : items) {
                delete item;
            }
        }
        items.clear();
    }
};

/**
 * Haiku object list information
 */
struct HaikuObjectList {
    std::vector<void*> objects;
    bool delete_on_remove;
    uint32_t id;
    
    HaikuObjectList() : delete_on_remove(false), id(0) {}
    
    ~HaikuObjectList() {
        Clear();
    }
    
    void Clear() {
        if (delete_on_remove) {
            for (auto obj : objects) {
                // In a real implementation, this would call delete on each object
                // For safety, we just clear the vector
            }
        }
        objects.clear();
    }
};

/**
 * Haiku locker information
 */
struct HaikuLocker {
    std::mutex mutex;
    bool is_locked;
    uint32_t lock_count;
    uint32_t owner_thread_id;
    uint32_t id;
    
    HaikuLocker() : is_locked(false), lock_count(0), owner_thread_id(0), id(0) {}
};

/**
 * Haiku point structure (2D coordinates)
 */
struct HaikuPoint {
    int32_t x;
    int32_t y;
    
    HaikuPoint(int32_t _x = 0, int32_t _y = 0) : x(_x), y(_y) {}
    
    // Point operations
    HaikuPoint operator+(const HaikuPoint& other) const {
        return HaikuPoint(x + other.x, y + other.y);
    }
    
    HaikuPoint operator-(const HaikuPoint& other) const {
        return HaikuPoint(x - other.x, y - other.y);
    }
    
    bool operator==(const HaikuPoint& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const HaikuPoint& other) const {
        return !(*this == other);
    }
};

/**
 * Haiku rectangle structure
 */
struct HaikuRect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    
    HaikuRect(int32_t l = 0, int32_t t = 0, int32_t r = 0, int32_t b = 0)
        : left(l), top(t), right(r), bottom(b) {}
    
    // Rectangle properties
    int32_t Width() const { return right - left; }
    int32_t Height() const { return bottom - top; }
    HaikuPoint LeftTop() const { return HaikuPoint(left, top); }
    HaikuPoint RightBottom() const { return HaikuPoint(right, bottom); }
    
    // Rectangle operations
    bool IsValid() const {
        return left <= right && top <= bottom;
    }
    
    bool Contains(const HaikuPoint& point) const {
        return point.x >= left && point.x <= right &&
               point.y >= top && point.y <= bottom;
    }
    
    bool Intersects(const HaikuRect& other) const {
        return !(right < other.left || left > other.right ||
                bottom < other.top || top > other.bottom);
    }
    
    HaikuRect Intersection(const HaikuRect& other) const {
        if (!Intersects(other)) {
            return HaikuRect();
        }
        
        return HaikuRect(
            std::max(left, other.left),
            std::max(top, other.top),
            std::min(right, other.right),
            std::min(bottom, other.bottom)
        );
    }
    
    HaikuRect Union(const HaikuRect& other) const {
        return HaikuRect(
            std::min(left, other.left),
            std::min(top, other.top),
            std::max(right, other.right),
            std::max(bottom, other.bottom)
        );
    }
    
    void OffsetBy(int32_t dx, int32_t dy) {
        left += dx;
        right += dx;
        top += dy;
        bottom += dy;
    }
    
    void OffsetBy(const HaikuPoint& offset) {
        OffsetBy(offset.x, offset.y);
    }
    
    void InsetBy(int32_t dx, int32_t dy) {
        left += dx;
        right -= dx;
        top += dy;
        bottom -= dy;
    }
    
    void Set(int32_t l, int32_t t, int32_t r, int32_t b) {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }
};

/**
 * Haiku size structure
 */
struct HaikuSize {
    float width;
    float height;
    
    HaikuSize(float w = 0.0f, float h = 0.0f) : width(w), height(h) {}
    
    bool operator==(const HaikuSize& other) const {
        return width == other.width && height == other.height;
    }
    
    bool operator!=(const HaikuSize& other) const {
        return !(*this == other);
    }
};

/**
 * Haiku message what codes (for Support Kit operations)
 */
#define HAIKU_SUPPORTKIT_MIN_WHAT        0x1000
#define HAIKU_SUPPORTKIT_MAX_WHAT        0x1FFF

// ============================================================================
// HAIKU SUPPORT KIT INTERFACE
// ============================================================================

/**
 * Haiku Support Kit implementation class
 * 
 * Provides complete Haiku support functionality including:
 * - BString: String manipulation and management
 * - BList: Generic list container
 * - BObjectList: Typed object list container  
 * - BLocker: Thread synchronization primitive
 * - Geometry: BPoint, BRect, BSize operations
 */
class HaikuSupportKitImpl : public HaikuKit {
private:
    // String management
    std::map<uint32_t, HaikuString*> strings;
    
    // List management
    std::map<uint32_t, HaikuList*> lists;
    
    // Object list management
    std::map<uint32_t, HaikuObjectList*> object_lists;
    
    // Locker management
    std::map<uint32_t, HaikuLocker*> lockers;
    
    // ID management
    uint32_t next_string_id;
    uint32_t next_list_id;
    uint32_t next_object_list_id;
    uint32_t next_locker_id;
    
    // Thread safety
    mutable std::mutex kit_mutex;
    bool initialized;
    
public:
    /**
     * Constructor
     */
    HaikuSupportKitImpl();
    
    /**
     * Destructor
     */
    virtual ~HaikuSupportKitImpl();
    
    // HaikuKit interface
    virtual status_t Initialize() override;
    virtual void Shutdown() override;
    
    /**
     * Get singleton instance
     */
    static HaikuSupportKitImpl& GetInstance();
    
    // ========================================================================
    // STRING OPERATIONS (BString)
    // ========================================================================
    
    /**
     * Create a new string
     */
    virtual uint32_t CreateString(const char* text = nullptr);
    
    /**
     * Set string content
     */
    virtual status_t SetString(uint32_t string_id, const char* text);
    
    /**
     * Append to string
     */
    virtual status_t AppendString(uint32_t string_id, const char* text);
    
    /**
     * Get string content
     */
    virtual status_t GetString(uint32_t string_id, char* buffer, size_t buffer_size);
    
    /**
     * Get string length
     */
    virtual size_t GetStringLength(uint32_t string_id) const;
    
    /**
     * Copy string
     */
    virtual uint32_t CopyString(uint32_t source_string_id);
    
    /**
     * Compare strings
     */
    virtual int32_t CompareStrings(uint32_t string1_id, uint32_t string2_id) const;
    
    /**
     * Find substring
     */
    virtual int32_t FindSubstring(uint32_t string_id, const char* substring) const;
    
    /**
     * Delete string
     */
    virtual void DeleteString(uint32_t string_id);
    
    // ========================================================================
    // LIST OPERATIONS (BList)
    // ========================================================================
    
    /**
     * Create a new list
     */
    virtual uint32_t CreateList(bool owns_items = true, bool delete_on_remove = true);
    
    /**
     * Add item to list
     */
    virtual status_t AddToList(uint32_t list_id, void* item, bool owns_data = false);
    
    /**
     * Remove item from list
     */
    virtual status_t RemoveFromList(uint32_t list_id, int32_t index);
    
    /**
     * Get item from list
     */
    virtual void* GetFromList(uint32_t list_id, int32_t index) const;
    
    /**
     * Count items in list
     */
    virtual int32_t CountListItems(uint32_t list_id) const;
    
    /**
     * Find item in list
     */
    virtual int32_t FindInList(uint32_t list_id, const void* item) const;
    
    /**
     * Clear list
     */
    virtual status_t ClearList(uint32_t list_id);
    
    /**
     * Delete list
     */
    virtual void DeleteList(uint32_t list_id);
    
    // ========================================================================
    // OBJECT LIST OPERATIONS (BObjectList)
    // ========================================================================
    
    /**
     * Create a new object list
     */
    virtual uint32_t CreateObjectList(bool delete_on_remove = false);
    
    /**
     * Add object to object list
     */
    virtual status_t AddToObjectList(uint32_t list_id, void* object);
    
    /**
     * Remove object from object list
     */
    virtual status_t RemoveFromObjectList(uint32_t list_id, int32_t index);
    
    /**
     * Get object from object list
     */
    virtual void* GetFromObjectList(uint32_t list_id, int32_t index) const;
    
    /**
     * Count objects in object list
     */
    virtual int32_t CountObjectListItems(uint32_t list_id) const;
    
    /**
     * Delete object list
     */
    virtual void DeleteObjectList(uint32_t list_id);
    
    // ========================================================================
    // LOCKER OPERATIONS (BLocker)
    // ========================================================================
    
    /**
     * Create a new locker
     */
    virtual uint32_t CreateLocker();
    
    /**
     * Acquire lock
     */
    virtual status_t AcquireLock(uint32_t locker_id);
    
    /**
     * Try to acquire lock (non-blocking)
     */
    virtual status_t TryLock(uint32_t locker_id);
    
    /**
     * Release lock
     */
    virtual status_t ReleaseLock(uint32_t locker_id);
    
    /**
     * Check if locked
     */
    virtual bool IsLocked(uint32_t locker_id) const;
    
    /**
     * Delete locker
     */
    virtual void DeleteLocker(uint32_t locker_id);
    
    // ========================================================================
    // GEOMETRY OPERATIONS (BPoint, BRect, BSize)
    // ========================================================================
    
    /**
     * Create point
     */
    virtual void CreatePoint(int32_t x, int32_t y, HaikuPoint* point);
    
    /**
     * Create rectangle
     */
    virtual void CreateRect(int32_t left, int32_t top, int32_t right, int32_t bottom,
                          HaikuRect* rect);
    
    /**
     * Create size
     */
    virtual void CreateSize(float width, float height, HaikuSize* size);
    
    /**
     * Rectangle operations
     */
    virtual bool RectContains(const HaikuRect& rect, const HaikuPoint& point) const;
    virtual bool RectIntersects(const HaikuRect& rect1, const HaikuRect& rect2) const;
    virtual void RectIntersection(const HaikuRect& rect1, const HaikuRect& rect2,
                                HaikuRect* result);
    virtual void RectUnion(const HaikuRect& rect1, const HaikuRect& rect2,
                         HaikuRect* result);
    virtual void OffsetRect(HaikuRect* rect, int32_t dx, int32_t dy);
    virtual void InsetRect(HaikuRect* rect, int32_t dx, int32_t dy);
    
    // ========================================================================
    // UTILITY METHODS
    // ========================================================================
    
    /**
     * Get support kit statistics
     */
    virtual void GetSupportStatistics(uint32_t* string_count, uint32_t* list_count,
                                    uint32_t* object_list_count, uint32_t* locker_count) const;
    
    /**
     * Dump support kit state for debugging
     */
    virtual void DumpSupportState() const;
    
private:
    /**
     * Resize string capacity
     */
    status_t ResizeString(HaikuString* string, size_t new_capacity);
    
    /**
     * Validate list index
     */
    bool IsValidListIndex(const HaikuList* list, int32_t index) const;
    
    /**
     * Validate object list index
     */
    bool IsValidObjectListIndex(const HaikuObjectList* list, int32_t index) const;
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

namespace HaikuSupportUtils {
    /**
     * String utilities
     */
    size_t StringLength(const char* str);
    char* StringDuplicate(const char* str);
    int32_t StringCompare(const char* str1, const char* str2);
    char* StringCopy(char* dest, const char* src, size_t max_size);
    
    /**
     * Memory utilities
     */
    void* MemoryAllocate(size_t size);
    void MemoryFree(void* ptr);
    void* MemoryReallocate(void* ptr, size_t new_size);
    
    /**
     * Hash utilities for string keys
     */
    size_t StringHash(const char* str);
    
    /**
     * Comparison utilities
     */
    int32_t PointerCompare(const void* ptr1, const void* ptr2);
    bool PointerEqual(const void* ptr1, const void* ptr2);
}