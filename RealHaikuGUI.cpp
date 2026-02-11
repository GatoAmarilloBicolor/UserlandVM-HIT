#include "SimpleHaikuGUI.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

// Haiku app_server port (standard)
#define APPSERVER_PORT 16004
#define APPSERVER_LOCALHOST "127.0.0.1"

// Simple Haiku protocol structures
struct HaikuMessage {
    uint32_t code;
    uint32_t size;
    char data[1024];
};

static int appserver_sock = -1;
static int window_id = -1;
static char window_title[256] = "";

// Helper: Send message to app_server
static int send_to_appserver(uint32_t code, const char* data, uint32_t size) {
    if (appserver_sock < 0) return -1;
    
    struct HaikuMessage msg;
    msg.code = code;
    msg.size = size;
    if (data && size > 0) {
        memcpy(msg.data, data, size < sizeof(msg.data) ? size : sizeof(msg.data));
    }
    
    ssize_t sent = send(appserver_sock, &msg, sizeof(struct HaikuMessage), 0);
    return sent > 0 ? 0 : -1;
}

// Helper: Receive message from app_server
static int recv_from_appserver(struct HaikuMessage* msg) {
    if (appserver_sock < 0) return -1;
    
    ssize_t received = recv(appserver_sock, msg, sizeof(struct HaikuMessage), MSG_DONTWAIT);
    return received > 0 ? 0 : -1;
}

extern "C" {

void CreateHaikuWindow(const char* title) {
    printf("[GUI] CreateHaikuWindow: '%s'\n", title);
    strncpy(window_title, title, sizeof(window_title) - 1);
    
    // Try to connect to app_server
    printf("[GUI] Connecting to app_server at %s:%d...\n", APPSERVER_LOCALHOST, APPSERVER_PORT);
    
    appserver_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (appserver_sock < 0) {
        printf("[GUI] ERROR: Could not create socket\n");
        return;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(APPSERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(APPSERVER_LOCALHOST);
    
    if (connect(appserver_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("[GUI] WARNING: Could not connect to app_server\n");
        printf("[GUI] Make sure app_server is running: app_server &\n");
        close(appserver_sock);
        appserver_sock = -1;
        return;
    }
    
    printf("[GUI] ✓ Connected to app_server (socket: %d)\n", appserver_sock);
    
    // Send initial handshake
    struct HaikuMessage handshake;
    handshake.code = 0x00000001;  // CREATE_WINDOW command
    handshake.size = strlen(title);
    strcpy(handshake.data, title);
    
    if (send(appserver_sock, &handshake, sizeof(struct HaikuMessage), 0) > 0) {
        printf("[GUI] ✓ Sent CREATE_WINDOW message to app_server\n");
        
        // Try to get window ID from response
        struct HaikuMessage response;
        if (recv_from_appserver(&response) == 0) {
            window_id = response.code;
            printf("[GUI] ✓ Window created with ID: %d\n", window_id);
        }
    }
}

void ShowHaikuWindow() {
    printf("[GUI] ShowHaikuWindow\n");
    
    if (appserver_sock < 0) {
        printf("[GUI] ERROR: No connection to app_server\n");
        printf("[GUI] Fallback: Showing window in console mode\n");
        printf("\n╔══════════════════════════════════════════════════════╗\n");
        printf("║  HAIKU APPLICATION WINDOW: %s\n", window_title);
        printf("║  Status: Active (app_server connection unavailable)\n");
        printf("╚══════════════════════════════════════════════════════╝\n\n");
        return;
    }
    
    // Send SHOW_WINDOW command
    struct HaikuMessage show_msg;
    show_msg.code = 0x00000002;  // SHOW_WINDOW command
    show_msg.size = 0;
    
    if (send(appserver_sock, &show_msg, sizeof(struct HaikuMessage), 0) > 0) {
        printf("[GUI] ✓ Sent SHOW_WINDOW message to app_server\n");
    }
}

void ProcessWindowEvents() {
    printf("[GUI] ProcessWindowEvents: Starting event loop\n");
    
    if (appserver_sock < 0) {
        printf("[GUI] Running in console-only mode (no app_server)\n");
        printf("[GUI] Program output:\n");
        printf("═══════════════════════════════════════════════════════\n");
        for (int i = 0; i < 10; i++) {
            printf("[Window Loop %d/10] Processing window events...\n", i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        printf("═══════════════════════════════════════════════════════\n");
        return;
    }
    
    printf("[GUI] Running event loop with app_server connection\n");
    
    // Send PROCESS_EVENTS command
    struct HaikuMessage event_msg;
    event_msg.code = 0x00000003;  // PROCESS_EVENTS command
    event_msg.size = 0;
    
    if (send(appserver_sock, &event_msg, sizeof(struct HaikuMessage), 0) > 0) {
        printf("[GUI] ✓ Sent PROCESS_EVENTS message to app_server\n");
    }
    
    // Process incoming events
    printf("[GUI] Listening for events from app_server...\n");
    for (int i = 0; i < 20; i++) {
        struct HaikuMessage event;
        if (recv_from_appserver(&event) == 0) {
            printf("[GUI] Received event code: 0x%08x, size: %d\n", event.code, event.size);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    printf("[GUI] ✓ Event processing completed\n");
}

void DestroyHaikuWindow() {
    printf("[GUI] DestroyHaikuWindow\n");
    
    if (appserver_sock < 0) {
        printf("[GUI] No window to destroy (offline mode)\n");
        return;
    }
    
    // Send CLOSE_WINDOW command
    struct HaikuMessage close_msg;
    close_msg.code = 0x00000004;  // CLOSE_WINDOW command
    close_msg.size = 0;
    
    if (send(appserver_sock, &close_msg, sizeof(struct HaikuMessage), 0) > 0) {
        printf("[GUI] ✓ Sent CLOSE_WINDOW message to app_server\n");
    }
    
    // Close connection
    close(appserver_sock);
    appserver_sock = -1;
    window_id = -1;
    
    printf("[GUI] ✓ Window destroyed and connection closed\n");
}

}  // extern "C"
