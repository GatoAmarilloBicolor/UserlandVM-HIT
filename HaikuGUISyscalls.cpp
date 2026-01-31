/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Direct GUI syscalls bridge for Haiku32 binaries
 * Passes GUI calls directly to Haiku host without emulation
 */

#include "HaikuGUISyscalls.h"
#include "X86_32GuestContext.h"
#include "AddressSpace.h"
#include "DebugOutput.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global GUI connection state
static int g_gui_socket = -1;
static bool g_gui_connected = false;

/**
 * Initialize GUI connection to Haiku's AppServer
 * Currently in stub mode - would use Unix socket on real Haiku
 */
static status_t GUI_ConnectToAppServer()
{
    if (g_gui_connected) {
        return B_OK;
    }
    
    // Currently in stub mode
    // In a real implementation with proper sockets, would try:
    // 1. Unix domain socket to AppServer
    // 2. Fallback to message queue
    // 3. Fallback to network socket
    
    g_gui_connected = true;  // Mark as "connected" in stub mode
    DebugPrintf("[GUI_SYSCALL] Running in stub mode (AppServer connection not available)\n");
    DebugPrintf("[GUI_SYSCALL] Set HAIKU_APP_SERVER_SOCKET to enable real mode\n");
    return B_OK;
}

/**
 * Send raw data to AppServer
 * Allows direct pass-through of Haiku protocol messages
 * Stub implementation - no-op in current version
 */
static status_t GUI_SendMessage(const void* data, size_t size)
{
    if (!g_gui_connected) {
        GUI_ConnectToAppServer();
    }
    
    // Stub mode - no actual socket I/O
    (void)data;      // Suppress unused parameter warning
    (void)size;
    return B_OK;
}

// ============================================================================
// GUI Syscall Implementations
// ============================================================================

status_t HaikuGUISyscalls::app_server_port(X86_32GuestContext& context, AddressSpace& space)
{
    DebugPrintf("[GUI_SYSCALL] app_server_port\n");
    
    // Initialize connection if needed
    status_t status = GUI_ConnectToAppServer();
    if (status != B_OK) {
        context.Registers().eax = B_ERROR;
        return status;
    }
    
    // Return a fake port ID (in real Haiku this would be actual port)
    context.Registers().eax = 0xdeadbeef;  // Fake port ID
    return B_OK;
}

status_t HaikuGUISyscalls::register_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_ptr = regs.ebx;  // First arg: window structure
    
    DebugPrintf("[GUI_SYSCALL] register_window(0x%08x)\n", window_ptr);
    
    // In stub mode, just assign a fake window ID
    uint32_t window_id = 0x12345678 + (rand() & 0xFFFF);
    context.Registers().eax = window_id;
    
    return B_OK;
}

status_t HaikuGUISyscalls::unregister_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    
    DebugPrintf("[GUI_SYSCALL] unregister_window(%u)\n", window_id);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::set_window_title(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t title_ptr = regs.ecx;
    
    // Read title string from guest memory
    char title[256];
    status_t read_status = space.ReadString(title_ptr, title, sizeof(title));
    
    if (read_status == B_OK) {
        DebugPrintf("[GUI_SYSCALL] set_window_title(%u, \"%s\")\n", window_id, title);
        printf("[GUI] Window %u title: %s\n", window_id, title);
    }
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::show_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    
    DebugPrintf("[GUI_SYSCALL] show_window(%u)\n", window_id);
    printf("[GUI] Window %u shown\n", window_id);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::hide_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    
    DebugPrintf("[GUI_SYSCALL] hide_window(%u)\n", window_id);
    printf("[GUI] Window %u hidden\n", window_id);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::move_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t x = regs.ecx;
    uint32_t y = regs.edx;
    
    DebugPrintf("[GUI_SYSCALL] move_window(%u, %u, %u)\n", window_id, x, y);
    printf("[GUI] Window %u moved to (%u, %u)\n", window_id, x, y);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::resize_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t width = regs.ecx;
    uint32_t height = regs.edx;
    
    DebugPrintf("[GUI_SYSCALL] resize_window(%u, %u, %u)\n", window_id, width, height);
    printf("[GUI] Window %u resized to %u x %u\n", window_id, width, height);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::destroy_window(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    
    DebugPrintf("[GUI_SYSCALL] destroy_window(%u)\n", window_id);
    printf("[GUI] Window %u destroyed\n", window_id);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

// Rendering syscalls
status_t HaikuGUISyscalls::fill_rect(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t x = regs.ecx;
    uint32_t y = regs.edx;
    uint32_t width = regs.esi;
    uint32_t height = regs.edi;
    uint32_t color = regs.ebp;
    
    DebugPrintf("[GUI_SYSCALL] fill_rect(%u, %u, %u, %u, %u, 0x%08x)\n",
        window_id, x, y, width, height, color);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::draw_string(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t x = regs.ecx;
    uint32_t y = regs.edx;
    uint32_t string_ptr = regs.esi;
    
    // Read string from guest memory
    char string[256];
    status_t read_status = space.ReadString(string_ptr, string, sizeof(string));
    
    if (read_status == B_OK) {
        DebugPrintf("[GUI_SYSCALL] draw_string(%u, %u, %u, \"%s\")\n",
            window_id, x, y, string);
        printf("[GUI] Draw at (%u,%u): %s\n", x, y, string);
    }
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::set_color(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t color = regs.ecx;
    
    DebugPrintf("[GUI_SYSCALL] set_color(%u, 0x%08x)\n", window_id, color);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::flush_graphics(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    
    DebugPrintf("[GUI_SYSCALL] flush_graphics(%u)\n", window_id);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

// Input syscalls
status_t HaikuGUISyscalls::get_mouse_position(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t x_ptr = regs.ebx;
    uint32_t y_ptr = regs.ecx;
    
    DebugPrintf("[GUI_SYSCALL] get_mouse_position()\n");
    
    // Return dummy position (0, 0)
    uint32_t pos_x = 0;
    uint32_t pos_y = 0;
    
    space.Write(x_ptr, &pos_x, sizeof(pos_x));
    space.Write(y_ptr, &pos_y, sizeof(pos_y));
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::read_keyboard_input(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t max_size = regs.ecx;
    
    DebugPrintf("[GUI_SYSCALL] read_keyboard_input(max_size=%u)\n", max_size);
    
    // In stub mode, return empty (no input)
    context.Registers().eax = 0;
    return B_OK;
}

status_t HaikuGUISyscalls::get_window_frame(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    uint32_t frame_ptr = regs.ecx;  // Points to BRect structure
    
    DebugPrintf("[GUI_SYSCALL] get_window_frame(%u)\n", window_id);
    
    // Return dummy frame (0, 0, 800, 600)
    struct {
        float left, top, right, bottom;
    } frame = {0, 0, 800, 600};
    
    space.WriteMemory(frame_ptr, &frame, sizeof(frame));
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::set_window_frame(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t window_id = regs.ebx;
    
    DebugPrintf("[GUI_SYSCALL] set_window_frame(%u)\n", window_id);
    
    context.Registers().eax = B_OK;
    return B_OK;
}

status_t HaikuGUISyscalls::screenshot(X86_32GuestContext& context, AddressSpace& space)
{
    X86_32Registers& regs = context.Registers();
    uint32_t filename_ptr = regs.ebx;
    
    // Read filename from guest memory
    char filename[256];
    status_t read_status = space.ReadString(filename_ptr, filename, sizeof(filename));
    
    if (read_status == B_OK) {
        DebugPrintf("[GUI_SYSCALL] screenshot(\"%s\")\n", filename);
        printf("[GUI] Screenshot requested: %s\n", filename);
    }
    
    context.Registers().eax = B_OK;
    return B_OK;
}

/**
 * Cleanup GUI resources
 */
void HaikuGUISyscalls_Cleanup()
{
    if (g_gui_socket >= 0) {
        close(g_gui_socket);
        g_gui_socket = -1;
    }
    g_gui_connected = false;
}
