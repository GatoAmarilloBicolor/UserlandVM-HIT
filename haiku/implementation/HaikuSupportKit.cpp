/*
 * HaikuSupportKit.cpp - Complete Haiku Support Kit Implementation
 * 
 * Implements all Haiku support utilities:
 * - BString: String manipulation, concatenation, comparison, searching
 * - BList: Generic list container with memory management
 * - BObjectList: Typed object list container
 * - BLocker: Thread synchronization primitive
 * - Geometry: BPoint, BRect, BSize operations
 */

#include "HaikuSupportKit.h"
#include "UnifiedStatusCodes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <thread>
#include <atomic>

// ============================================================================
// UTILITY IMPLEMENTATIONS
// ============================================================================

namespace HaikuSupportUtils {
    
size_t StringLength(const char* str) {
    return str ? strlen(str) : 0;
}

char* StringDuplicate(const char* str) {
    if (!str) return nullptr;
    
    size_t len = strlen(str);
    char* result = static_cast<char*>(malloc(len + 1));
    if (result) {
        strcpy(result, str);
    }
    return result;
}

int32_t StringCompare(const char* str1, const char* str2) {
    if (!str1 && !str2) return 0;
    if (!str1) return -1;
    if (!str2) return 1;
    return strcmp(str1, str2);
}

char* StringCopy(char* dest, const char* src, size_t max_size) {
    if (!dest || !src || max_size == 0) return nullptr;
    
    size_t src_len = strlen(src);
    size_t copy_len = std::min(src_len, max_size - 1);
    strncpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return dest;
}

void* MemoryAllocate(size_t size) {
    return malloc(size);
}

void MemoryFree(void* ptr) {
    free(ptr);
}

void* MemoryReallocate(void* ptr, size_t new_size) {
    return realloc(ptr, new_size);
}

size_t StringHash(const char* str) {
    if (!str) return 0;
    
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int32_t PointerCompare(const void* ptr1, const void* ptr2) {
    if (ptr1 < ptr2) return -1;
    if (ptr1 > ptr2) return 1;
    return 0;
}

bool PointerEqual(const void* ptr1, const void* ptr2) {
    return ptr1 == ptr2;
}

} // namespace HaikuSupportUtils

// ============================================================================
// HAIKU SUPPORT KIT IMPLEMENTATION
// ============================================================================

// Static instance initialization
HaikuSupportKitImpl* HaikuSupportKitImpl::instance = nullptr;
std::mutex HaikuSupportKitImpl::instance_mutex;

HaikuSupportKitImpl& HaikuSupportKitImpl::GetInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = new HaikuSupportKitImpl();
    }
    return *instance;
}

HaikuSupportKitImpl::HaikuSupportKitImpl() : HaikuKit("Support Kit") {
    next_string_id = 1;
    next_list_id = 1;
    next_object_list_id = 1;
    next_locker_id = 1;
    
    printf("[HAIKU_SUPPORT] Initializing Support Kit...\n");
}

HaikuSupportKitImpl::~HaikuSupportKitImpl() {
    if (initialized) {
        Shutdown();
    }
    delete instance;
    instance = nullptr;
}

status_t HaikuSupportKitImpl::Initialize() {
    if (initialized) {
        return B_OK;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_SUPPORT] ✅ Support Kit initialized\n");
    printf("[HAIKU_SUPPORT] 📝 String system ready\n");
    printf("[HAIKU_SUPPORT] 📋 List system ready\n");
    printf("[HAIKU_SUPPORT] 🔒 Locker system ready\n");
    printf("[HAIKU_SUPPORT] 📐 Geometry system ready\n");
    
    initialized = true;
    return B_OK;
}

void HaikuSupportKitImpl::Shutdown() {
    if (!initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_SUPPORT] Shutting down Support Kit...\n");
    
    // Delete all strings
    for (auto& pair : strings) {
        delete pair.second;
    }
    strings.clear();
    
    // Delete all lists
    for (auto& pair : lists) {
        delete pair.second;
    }
    lists.clear();
    
    // Delete all object lists
    for (auto& pair : object_lists) {
        delete pair.second;
    }
    object_lists.clear();
    
    // Delete all lockers
    for (auto& pair : lockers) {
        delete pair.second;
    }
    lockers.clear();
    
    initialized = false;
    
    printf("[HAIKU_SUPPORT] ✅ Support Kit shutdown complete\n");
}

// ============================================================================
// STRING OPERATIONS (BString)
// ============================================================================

uint32_t HaikuSupportKitImpl::CreateString(const char* text) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    uint32_t string_id = next_string_id++;
    
    HaikuString* haiku_string = new HaikuString();
    haiku_string->id = string_id;
    
    if (text) {
        size_t text_len = strlen(text);
        haiku_string->capacity = text_len + HAIKU_STRING_INITIAL_SIZE;
        haiku_string->data = static_cast<char*>(malloc(haiku_string->capacity));
        if (haiku_string->data) {
            strcpy(haiku_string->data, text);
            haiku_string->length = text_len;
        }
    }
    
    strings[string_id] = haiku_string;
    
    printf("[HAIKU_SUPPORT] 📝 Created string %u: \"%s\"\n", 
           string_id, text ? text : "(null)");
    
    return string_id;
}

status_t HaikuSupportKitImpl::SetString(uint32_t string_id, const char* text) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(string_id);
    if (it == strings.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuString* haiku_string = it->second;
    size_t text_len = text ? strlen(text) : 0;
    
    if (text_len + 1 > haiku_string->capacity) {
        if (!ResizeString(haiku_string, text_len + HAIKU_STRING_INITIAL_SIZE)) {
            return B_NO_MEMORY;
        }
    }
    
    if (text) {
        strcpy(haiku_string->data, text);
        haiku_string->length = text_len;
    } else {
        haiku_string->data[0] = '\0';
        haiku_string->length = 0;
    }
    
    printf("[HAIKU_SUPPORT] 📝 Set string %u to \"%s\"\n", string_id, text);
    
    return B_OK;
}

status_t HaikuSupportKitImpl::AppendString(uint32_t string_id, const char* text) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(string_id);
    if (it == strings.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuString* haiku_string = it->second;
    
    if (!text) {
        return B_OK;
    }
    
    size_t text_len = strlen(text);
    size_t new_len = haiku_string->length + text_len;
    
    if (new_len + 1 > haiku_string->capacity) {
        size_t new_capacity = new_len * HAIKU_STRING_GROWTH_FACTOR;
        if (!ResizeString(haiku_string, new_capacity)) {
            return B_NO_MEMORY;
        }
    }
    
    strcat(haiku_string->data, text);
    haiku_string->length = new_len;
    
    printf("[HAIKU_SUPPORT] 📝 Appended to string %u: \"%s\"\n", string_id, text);
    
    return B_OK;
}

status_t HaikuSupportKitImpl::GetString(uint32_t string_id, char* buffer, size_t buffer_size) {
    if (!initialized || !buffer || buffer_size == 0) {
        return B_BAD_VALUE;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(string_id);
    if (it == strings.end() || !it->second->data) {
        buffer[0] = '\0';
        return B_BAD_VALUE;
    }
    
    HaikuString* haiku_string = it->second;
    size_t copy_len = std::min(haiku_string->length, buffer_size - 1);
    strncpy(buffer, haiku_string->data, copy_len);
    buffer[copy_len] = '\0';
    
    return B_OK;
}

size_t HaikuSupportKitImpl::GetStringLength(uint32_t string_id) const {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(string_id);
    if (it != strings.end()) {
        return it->second->length;
    }
    
    return 0;
}

uint32_t HaikuSupportKitImpl::CopyString(uint32_t source_string_id) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(source_string_id);
    if (it == strings.end() || !it->second->data) {
        return 0;
    }
    
    return CreateString(it->second->data);
}

int32_t HaikuSupportKitImpl::CompareStrings(uint32_t string1_id, uint32_t string2_id) const {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it1 = strings.find(string1_id);
    auto it2 = strings.find(string2_id);
    
    const char* str1 = (it1 != strings.end() && it1->second->data) ? it1->second->data : "";
    const char* str2 = (it2 != strings.end() && it2->second->data) ? it2->second->data : "";
    
    return HaikuSupportUtils::StringCompare(str1, str2);
}

int32_t HaikuSupportKitImpl::FindSubstring(uint32_t string_id, const char* substring) const {
    if (!initialized || !substring) return -1;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(string_id);
    if (it == strings.end() || !it->second->data) {
        return -1;
    }
    
    const char* str = it->second->data;
    const char* found = strstr(str, substring);
    if (!found) {
        return -1;
    }
    
    return found - str;
}

void HaikuSupportKitImpl::DeleteString(uint32_t string_id) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = strings.find(string_id);
    if (it != strings.end()) {
        printf("[HAIKU_SUPPORT] 🗑️  Deleted string %u\n", string_id);
        delete it->second;
        strings.erase(it);
    }
}

// ============================================================================
// LIST OPERATIONS (BList)
// ============================================================================

uint32_t HaikuSupportKitImpl::CreateList(bool owns_items, bool delete_on_remove) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    uint32_t list_id = next_list_id++;
    
    HaikuList* haiku_list = new HaikuList();
    haiku_list->id = list_id;
    haiku_list->owns_items = owns_items;
    haiku_list->item_ownership = delete_on_remove;
    
    lists[list_id] = haiku_list;
    
    printf("[HAIKU_SUPPORT] 📋 Created list %u (owns_items=%s, delete_on_remove=%s)\n",
           list_id, owns_items ? "true" : "false", delete_on_remove ? "true" : "false");
    
    return list_id;
}

status_t HaikuSupportKitImpl::AddToList(uint32_t list_id, void* item, bool owns_data) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it == lists.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuList* haiku_list = it->second;
    HaikuListItem* list_item = new HaikuListItem();
    list_item->data = item;
    list_item->owns_data = owns_data;
    list_item->id = haiku_list->items.size();
    
    haiku_list->items.push_back(list_item);
    
    printf("[HAIKU_SUPPORT] 📋 Added item %p to list %u (owns_data=%s)\n",
           item, list_id, owns_data ? "true" : "false");
    
    return B_OK;
}

status_t HaikuSupportKitImpl::RemoveFromList(uint32_t list_id, int32_t index) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it == lists.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuList* haiku_list = it->second;
    if (!IsValidListIndex(haiku_list, index)) {
        return B_BAD_VALUE;
    }
    
    HaikuListItem* item = haiku_list->items[index];
    delete item;
    haiku_list->items.erase(haiku_list->items.begin() + index);
    
    printf("[HAIKU_SUPPORT] 📋 Removed item %d from list %u\n", index, list_id);
    
    return B_OK;
}

void* HaikuSupportKitImpl::GetFromList(uint32_t list_id, int32_t index) const {
    if (!initialized) return nullptr;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it == lists.end()) {
        return nullptr;
    }
    
    const HaikuList* haiku_list = it->second;
    if (!IsValidListIndex(haiku_list, index)) {
        return nullptr;
    }
    
    return haiku_list->items[index]->data;
}

int32_t HaikuSupportKitImpl::CountListItems(uint32_t list_id) const {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it != lists.end()) {
        return static_cast<int32_t>(it->second->items.size());
    }
    
    return 0;
}

int32_t HaikuSupportKitImpl::FindInList(uint32_t list_id, const void* item) const {
    if (!initialized || !item) return -1;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it == lists.end()) {
        return -1;
    }
    
    const HaikuList* haiku_list = it->second;
    for (size_t i = 0; i < haiku_list->items.size(); i++) {
        if (haiku_list->items[i]->data == item) {
            return static_cast<int32_t>(i);
        }
    }
    
    return -1;
}

status_t HaikuSupportKitImpl::ClearList(uint32_t list_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it == lists.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuList* haiku_list = it->second;
    haiku_list->Clear();
    
    printf("[HAIKU_SUPPORT] 📋 Cleared list %u\n", list_id);
    
    return B_OK;
}

void HaikuSupportKitImpl::DeleteList(uint32_t list_id) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lists.find(list_id);
    if (it != lists.end()) {
        printf("[HAIKU_SUPPORT] 🗑️  Deleted list %u\n", list_id);
        delete it->second;
        lists.erase(it);
    }
}

// ============================================================================
// OBJECT LIST OPERATIONS (BObjectList)
// ============================================================================

uint32_t HaikuSupportKitImpl::CreateObjectList(bool delete_on_remove) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    uint32_t object_list_id = next_object_list_id++;
    
    HaikuObjectList* object_list = new HaikuObjectList();
    object_list->id = object_list_id;
    object_list->delete_on_remove = delete_on_remove;
    
    object_lists[object_list_id] = object_list;
    
    printf("[HAIKU_SUPPORT] 📋 Created object list %u (delete_on_remove=%s)\n",
           object_list_id, delete_on_remove ? "true" : "false");
    
    return object_list_id;
}

status_t HaikuSupportKitImpl::AddToObjectList(uint32_t list_id, void* object) {
    if (!initialized || !object) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = object_lists.find(list_id);
    if (it == object_lists.end()) {
        return B_BAD_VALUE;
    }
    
    it->second->objects.push_back(object);
    
    printf("[HAIKU_SUPPORT] 📋 Added object %p to object list %u\n", object, list_id);
    
    return B_OK;
}

status_t HaikuSupportKitImpl::RemoveFromObjectList(uint32_t list_id, int32_t index) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = object_lists.find(list_id);
    if (it == object_lists.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuObjectList* object_list = it->second;
    if (!IsValidObjectListIndex(object_list, index)) {
        return B_BAD_VALUE;
    }
    
    object_list->objects.erase(object_list->objects.begin() + index);
    
    printf("[HAIKU_SUPPORT] 📋 Removed object %d from object list %u\n", index, list_id);
    
    return B_OK;
}

void* HaikuSupportKitImpl::GetFromObjectList(uint32_t list_id, int32_t index) const {
    if (!initialized) return nullptr;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = object_lists.find(list_id);
    if (it == object_lists.end()) {
        return nullptr;
    }
    
    const HaikuObjectList* object_list = it->second;
    if (!IsValidObjectListIndex(object_list, index)) {
        return nullptr;
    }
    
    return object_list->objects[index];
}

int32_t HaikuSupportKitImpl::CountObjectListItems(uint32_t list_id) const {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = object_lists.find(list_id);
    if (it != object_lists.end()) {
        return static_cast<int32_t>(it->second->objects.size());
    }
    
    return 0;
}

void HaikuSupportKitImpl::DeleteObjectList(uint32_t list_id) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = object_lists.find(list_id);
    if (it != object_lists.end()) {
        printf("[HAIKU_SUPPORT] 🗑️  Deleted object list %u\n", list_id);
        delete it->second;
        object_lists.erase(it);
    }
}

// ============================================================================
// LOCKER OPERATIONS (BLocker)
// ============================================================================

uint32_t HaikuSupportKitImpl::CreateLocker() {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    uint32_t locker_id = next_locker_id++;
    
    HaikuLocker* locker = new HaikuLocker();
    locker->id = locker_id;
    
    lockers[locker_id] = locker;
    
    printf("[HAIKU_SUPPORT] 🔒 Created locker %u\n", locker_id);
    
    return locker_id;
}

status_t HaikuSupportKitImpl::AcquireLock(uint32_t locker_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lockers.find(locker_id);
    if (it == lockers.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuLocker* locker = it->second;
    locker->mutex.lock();
    locker->is_locked = true;
    locker->lock_count++;
    locker->owner_thread_id = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    
    printf("[HAIKU_SUPPORT] 🔒 Acquired locker %u (count: %u, thread: %u)\n",
           locker_id, locker->lock_count, locker->owner_thread_id);
    
    return B_OK;
}

status_t HaikuSupportKitImpl::TryLock(uint32_t locker_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lockers.find(locker_id);
    if (it == lockers.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuLocker* locker = it->second;
    
    if (locker->mutex.try_lock()) {
        locker->is_locked = true;
        locker->lock_count++;
        locker->owner_thread_id = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        
        printf("[HAIKU_SUPPORT] 🔒 Try lock succeeded on locker %u\n", locker_id);
        return B_OK;
    }
    
    printf("[HAIKU_SUPPORT] 🔒 Try lock failed on locker %u\n", locker_id);
    return B_ERROR;
}

status_t HaikuSupportKitImpl::ReleaseLock(uint32_t locker_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lockers.find(locker_id);
    if (it == lockers.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuLocker* locker = it->second;
    if (!locker->is_locked) {
        return B_ERROR;
    }
    
    locker->mutex.unlock();
    locker->is_locked = false;
    locker->lock_count = 0;
    locker->owner_thread_id = 0;
    
    printf("[HAIKU_SUPPORT] 🔓 Released locker %u\n", locker_id);
    
    return B_OK;
}

bool HaikuSupportKitImpl::IsLocked(uint32_t locker_id) const {
    if (!initialized) return false;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lockers.find(locker_id);
    if (it != lockers.end()) {
        return it->second->is_locked;
    }
    
    return false;
}

void HaikuSupportKitImpl::DeleteLocker(uint32_t locker_id) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = lockers.find(locker_id);
    if (it != lockers.end()) {
        printf("[HAIKU_SUPPORT] 🗑️  Deleted locker %u\n", locker_id);
        delete it->second;
        lockers.erase(it);
    }
}

// ============================================================================
// GEOMETRY OPERATIONS (BPoint, BRect, BSize)
// ============================================================================

void HaikuSupportKitImpl::CreatePoint(int32_t x, int32_t y, HaikuPoint* point) {
    if (point) {
        *point = HaikuPoint(x, y);
    }
}

void HaikuSupportKitImpl::CreateRect(int32_t left, int32_t top, int32_t right, int32_t bottom,
                                   HaikuRect* rect) {
    if (rect) {
        *rect = HaikuRect(left, top, right, bottom);
    }
}

void HaikuSupportKitImpl::CreateSize(float width, float height, HaikuSize* size) {
    if (size) {
        *size = HaikuSize(width, height);
    }
}

bool HaikuSupportKitImpl::RectContains(const HaikuRect& rect, const HaikuPoint& point) const {
    return rect.Contains(point);
}

bool HaikuSupportKitImpl::RectIntersects(const HaikuRect& rect1, const HaikuRect& rect2) const {
    return rect1.Intersects(rect2);
}

void HaikuSupportKitImpl::RectIntersection(const HaikuRect& rect1, const HaikuRect& rect2,
                                         HaikuRect* result) {
    if (result) {
        *result = rect1.Intersection(rect2);
    }
}

void HaikuSupportKitImpl::RectUnion(const HaikuRect& rect1, const HaikuRect& rect2,
                                  HaikuRect* result) {
    if (result) {
        *result = rect1.Union(rect2);
    }
}

void HaikuSupportKitImpl::OffsetRect(HaikuRect* rect, int32_t dx, int32_t dy) {
    if (rect) {
        rect->OffsetBy(dx, dy);
    }
}

void HaikuSupportKitImpl::InsetRect(HaikuRect* rect, int32_t dx, int32_t dy) {
    if (rect) {
        rect->InsetBy(dx, dy);
    }
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

status_t HaikuSupportKitImpl::ResizeString(HaikuString* string, size_t new_capacity) {
    if (!string) return B_BAD_VALUE;
    
    char* new_data = static_cast<char*>(realloc(string->data, new_capacity));
    if (!new_data) {
        return B_NO_MEMORY;
    }
    
    string->data = new_data;
    string->capacity = new_capacity;
    
    return B_OK;
}

bool HaikuSupportKitImpl::IsValidListIndex(const HaikuList* list, int32_t index) const {
    if (!list || index < 0) {
        return false;
    }
    
    return static_cast<size_t>(index) < list->items.size();
}

bool HaikuSupportKitImpl::IsValidObjectListIndex(const HaikuObjectList* list, int32_t index) const {
    if (!list || index < 0) {
        return false;
    }
    
    return static_cast<size_t>(index) < list->objects.size();
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

void HaikuSupportKitImpl::GetSupportStatistics(uint32_t* string_count, uint32_t* list_count,
                                               uint32_t* object_list_count, uint32_t* locker_count) const {
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (string_count) *string_count = strings.size();
    if (list_count) *list_count = lists.size();
    if (object_list_count) *object_list_count = object_lists.size();
    if (locker_count) *locker_count = lockers.size();
}

void HaikuSupportKitImpl::DumpSupportState() const {
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_SUPPORT] Support Kit State Dump:\n");
    printf("  Strings: %zu\n", strings.size());
    printf("  Lists: %zu\n", lists.size());
    printf("  Object Lists: %zu\n", object_lists.size());
    printf("  Lockers: %zu\n", lockers.size());
    
    printf("  String Details:\n");
    for (const auto& pair : strings) {
        const HaikuString* haiku_string = pair.second;
        printf("    %u: \"%.*s\" (%zu bytes, %zu capacity)\n",
               pair.first,
               static_cast<int>(std::min(haiku_string->length, static_cast<size_t>(32))),
               haiku_string->data ? haiku_string->data : "(null)",
               haiku_string->length, haiku_string->capacity);
    }
    
    printf("  List Details:\n");
    for (const auto& pair : lists) {
        const HaikuList* haiku_list = pair.second;
        printf("    %u: %zu items (owns_items=%s)\n",
               pair.first, haiku_list->items.size(),
               haiku_list->owns_items ? "true" : "false");
    }
    
    printf("  Locker Details:\n");
    for (const auto& pair : lockers) {
        const HaikuLocker* locker = pair.second;
        printf("    %u: %s (locked=%s, count=%u, thread=%u)\n",
               pair.first,
               locker->is_locked ? "locked" : "unlocked",
               locker->is_locked ? "true" : "false",
               locker->lock_count, locker->owner_thread_id);
    }
}

// C compatibility wrapper
extern "C" {
    HaikuSupportKit* GetHaikuSupportKit() {
        return &HaikuSupportKitImpl::GetInstance();
    }
}