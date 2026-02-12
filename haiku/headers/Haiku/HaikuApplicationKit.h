/*
 * HaikuApplicationKit.h - Complete Haiku Application Kit Interface
 * 
 * Interface for all Haiku application operations: BApplication, BLooper, BMessenger, BMessage, BHandler
 */

#pragma once

#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <string>
#include <mutex>
#include <map>
#include <vector>

// Haiku Application Kit constants
#define HAIKU_MAX_MESSAGES           1024
#define HAIKU_MAX_HANDLERS           256
#define HAIKU_MAX_LOOPERS           64
#define HAIKU_MAX_MESSENGERS       32
#define HAIKU_MAX_WHAT              64
#define HAIKU_MAX_SPECIFIER        64
#define HAIKU_MAX_TARGET             64
#define HAIKU_MAX_REPLY             1024
#define HAIKU_MAX_FILE_PATH        1024

// Haiku message types
#define HAIKU_MESSAGE_TYPE_APP_QUIT         1
#define HAIKU_MESSAGE_TYPE_APP_HIDDEN        2
#define HAIKU_MESSAGE_TYPE_APP_ACTIVATED      3
#define HAIKU_MESSAGE_TYPE_APP_DEACTIVATED    4
#define HAIKU_MESSAGE_TYPE_CUSTOM            1000

// Haiku message delivery states
#define HAIKU_MESSAGE_DELIVERY_SUCCESS       0
#define HAIKU_MESSAGE_DELIVERY_IN_PROGRESS    1
#define HAIKU_MESSAGE_DELIVERY_FAILED        2
#define HAIKU_MESSAGE_DELIVERY_NO_TARGET     3

// ============================================================================
// HAIKU APPLICATION KIT DATA STRUCTURES
// ============================================================================

/**
 * Haiku message structure
 */
struct HaikuMessage {
    uint32_t id;
    uint32_t message_type;
    uint32_t what_code;
    uint64_t when;
    void* data;
    size_t data_size;
    uint32_t reply_target;
    uint32_t reply_id;
    bool is_reply;
    bool is_source_waiting;
    
    HaikuMessage() : id(0), message_type(0), what_code(0), when(0),
                     data(nullptr), data_size(0), reply_target(0), reply_id(0),
                     is_reply(false), is_source_waiting(false) {}
};

/**
 * Haiku message filter structure
 */
struct HaikuMessageFilter {
    uint32_t message_types[HAIKU_MAX_WHAT];
    uint32_t count;
    bool include_all;
    
    HaikuMessageFilter() : count(0), include_all(false) {
        for (size_t i = 0; i < HAIKU_MAX_WHAT; i++) {
            message_types[i] = 0;
        }
    }
};

/**
 * Haiku messenger information
 */
struct HaikuMessenger {
    uint32_t id;
    uint32_t target_type;
    uint32_t target_id;
    char signature[64];
    bool is_valid;
    bool prefer_local;
    
    HaikuMessenger() : id(0), target_type(0), target_id(0), is_valid(false),
                      prefer_local(true) {
        memset(signature, 0, sizeof(signature));
    }
};

/**
 * Haiku looper information
 */
struct HaikuLooper {
    uint32_t id;
    char name[64];
    uint32_t message_count;
    uint32_t handler_count;
    bool is_running;
    bool is_locked;
    uint32_t message_queue[HAIKU_MAX_MESSAGES];
    uint32_t queue_head;
    uint32_t queue_tail;
    
    HaikuLooper() : id(0), message_count(0), handler_count(0), is_running(false),
                 is_locked(false), queue_head(0), queue_tail(0) {
        for (size_t i = 0; i < sizeof(name); i++) name[i] = 0;
        for (size_t i = 0; i < HAIKU_MAX_MESSAGES; i++) {
            message_queue[i] = 0;
        }
    }
};

/**
 * Haiku handler information
 */
struct HaikuHandler {
    uint32_t id;
    char name[64];
    uint32_t looper_id;
    uint32_t message_filter_id;
    bool is_active;
    uint32_t message_count;
    
    HaikuHandler() : id(0), looper_id(0), message_filter_id(0), is_active(false),
                   message_count(0) {
        for (size_t i = 0; i < sizeof(name); i++) name[i] = 0;
    }
};

/**
 * Haiku application information
 */
struct HaikuApplicationInfo {
    char signature[64];
    uint32_t app_id;
    uint32_t main_looper_id;
    uint32_t looper_count;
    bool is_running;
    bool is_quit_requested;
    uint32_t message_count;
    time_t start_time;
    
    HaikuApplicationInfo() : app_id(0), main_looper_id(0), looper_count(0),
                           is_running(false), is_quit_requested(false), message_count(0),
                           start_time(0) {
        for (size_t i = 0; i < sizeof(signature); i++) signature[i] = 0;
    }
};

// ============================================================================
// HAIKU APPLICATION KIT INTERFACE
// ============================================================================

/**
 * Haiku Application Kit implementation class
 * 
 * Provides complete Haiku application functionality including:
 * - BApplication lifecycle management
 * - Message passing and filtering
 * - Looper and handler management
 * - Messenger inter-process communication
 */
class HaikuApplicationKitImpl : public HaikuKit {
private:
    // Message management
    std::map<uint32_t, HaikuMessage> messages;
    std::map<uint32_t, HaikuMessageFilter> message_filters;
    std::map<uint32_t, HaikuMessenger> messengers;
    
    // Looper and handler management
    std::map<uint32_t, HaikuLooper> loopers;
    std::map<uint32_t, HaikuHandler> handlers;
    
    // Application state
    HaikuApplicationInfo app_info;
    
    // ID management
    uint32_t next_message_id;
    uint32_t next_looper_id;
    uint32_t next_handler_id;
    uint32_t next_messenger_id;
    uint32_t next_filter_id;
    
    // Thread safety
    mutable std::mutex kit_mutex;
    bool initialized;
    
public:
    /**
     * Constructor
     */
    HaikuApplicationKit() : HaikuKit("Application Kit") {
        next_message_id = 1;
        next_looper_id = 1;
        next_handler_id = 1;
        next_messenger_id = 1;
        next_filter_id = 1;
        
        // Initialize app info
        memset(&app_info, 0, sizeof(app_info));
        strcpy(app_info.signature, "application/x-vnd.UnknownApplication");
    }
    
    /**
     * Destructor
     */
    virtual ~HaikuApplicationKit() {
        if (initialized) {
            Shutdown();
        }
    }
    
    // HaikuKit interface
    virtual status_t Initialize() override;
    virtual void Shutdown() override;
    
    /**
     * Get singleton instance
     */
    static HaikuApplicationKit& GetInstance();
    
    // ========================================================================
    // APPLICATION MANAGEMENT
    // ========================================================================
    
    /**
     * Create and initialize Haiku application
     */
    virtual status_t CreateApplication(const char* signature);
    
    /**
     * Start application message loops
     */
    virtual status_t RunApplication();
    
    /**
     * Request application to quit
     */
    virtual status_t QuitApplication();
    
    /**
     * Check if application is running
     */
    virtual bool IsApplicationRunning() const;
    
    /**
     * Get application information
     */
    virtual const HaikuApplicationInfo& GetApplicationInfo() const;
    
    // ========================================================================
    // MESSAGE MANAGEMENT
    // ========================================================================
    
    /**
     * Create a new message
     */
    virtual uint32_t CreateMessage(uint32_t message_type, uint32_t what_code,
                                  const void* data, size_t data_size);
    
    /**
     * Send message to target
     */
    virtual status_t SendMessage(uint32_t message_id, uint32_t target_looper_id,
                             uint32_t target_handler_id);
    
    /**
     * Send message with reply
     */
    virtual status_t SendMessageWithReply(uint32_t message_id, uint32_t target_looper_id,
                                        uint32_t target_handler_id, uint32_t reply_target);
    
    /**
     * Post message to message queue
     */
    virtual status_t PostMessage(uint32_t message_id, uint32_t target_looper_id);
    
    /**
     * Get message by ID
     */
    virtual const HaikuMessage* GetMessage(uint32_t message_id) const;
    
    /**
     * Delete message
     */
    virtual void DeleteMessage(uint32_t message_id);
    
    // ========================================================================
    // LOOPER MANAGEMENT
    // ========================================================================
    
    /**
     * Create a new looper
     */
    virtual uint32_t CreateLooper(const char* name);
    
    /**
     * Run looper message loop
     */
    virtual status_t RunLooper(uint32_t looper_id);
    
    /**
     * Quit looper
     */
    virtual status_t QuitLooper(uint32_t looper_id);
    
    /**
     * Get looper by ID
     */
    virtual const HaikuLooper* GetLooper(uint32_t looper_id) const;
    
    /**
     * Delete looper
     */
    virtual void DeleteLooper(uint32_t looper_id);
    
    // ========================================================================
    // HANDLER MANAGEMENT
    // ========================================================================
    
    /**
     * Create a new handler
     */
    virtual uint32_t CreateHandler(const char* name, uint32_t looper_id);
    
    /**
     * Add handler to looper
     */
    virtual status_t AddHandlerToLooper(uint32_t handler_id, uint32_t looper_id);
    
    /**
     * Remove handler from looper
     */
    virtual status_t RemoveHandlerFromLooper(uint32_t handler_id, uint32_t looper_id);
    
    /**
     * Get handler by ID
     */
    virtual const HaikuHandler* GetHandler(uint32_t handler_id) const;
    
    /**
     * Delete handler
     */
    virtual void DeleteHandler(uint32_t handler_id);
    
    // ========================================================================
    // MESSENGER MANAGEMENT
    // ========================================================================
    
    /**
     * Create messenger to target
     */
    virtual uint32_t CreateMessenger(const char* signature);
    
    /**
     * Send message via messenger
     */
    virtual status_t SendViaMessenger(uint32_t messenger_id, const HaikuMessage* message);
    
    /**
     * Get messenger by ID
     */
    virtual const HaikuMessenger* GetMessenger(uint32_t messenger_id) const;
    
    /**
     * Delete messenger
     */
    virtual void DeleteMessenger(uint32_t messenger_id);
    
    // ========================================================================
    // MESSAGE FILTERING
    // ========================================================================
    
    /**
     * Create message filter
     */
    virtual uint32_t CreateMessageFilter(const uint32_t* what_codes, uint32_t count,
                                         bool include_all = false);
    
    /**
     * Set filter for handler
     */
    virtual status_t SetHandlerMessageFilter(uint32_t handler_id, uint32_t filter_id);
    
    /**
     * Get filter by ID
     */
    virtual const HaikuMessageFilter* GetMessageFilter(uint32_t filter_id) const;
    
    /**
     * Delete message filter
     */
    virtual void DeleteMessageFilter(uint32_t filter_id);
    
    // ========================================================================
    // UTILITY METHODS
    // ========================================================================
    
    /**
     * Get application kit statistics
     */
    virtual void GetApplicationStatistics(uint32_t* message_count,
                                       uint32_t* looper_count,
                                       uint32_t* handler_count,
                                       uint32_t* messenger_count) const;
    
    /**
     * Dump application kit state for debugging
     */
    virtual void DumpApplicationState() const;
    
private:
    /**
     * Process message in looper
     */
    status_t ProcessMessageInLooper(uint32_t looper_id, uint32_t message_id);
    
    /**
     * Handle message in handler
     */
    status_t HandleMessageInHandler(uint32_t handler_id, uint32_t message_id);
    
    /**
     * Broadcast message to all handlers
     */
    status_t BroadcastMessage(uint32_t message_id);
    
    /**
     * Check message delivery state
     */
    bool CanDeliverMessage(const HaikuMessage* message, uint32_t target_looper_id,
                          uint32_t target_handler_id);
    
    /**
     * Enqueue message in looper queue
     */
    status_t EnqueueMessageInLooper(uint32_t looper_id, uint32_t message_id);
    
    /**
     * Dequeue message from looper queue
     */
    uint32_t DequeueMessageFromLooper(uint32_t looper_id);
};