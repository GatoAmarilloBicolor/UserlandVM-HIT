/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>

// Forward declarations
class AddressSpace;
class X86_32GuestContext;

/**
 * GUI-related syscalls for Haiku32 binaries
 * These are stub implementations that allow GUI programs to run
 * without actually rendering to the host display
 */
class HaikuGUISyscalls {
public:
	// Application syscalls
	static status_t app_server_port(X86_32GuestContext& context, AddressSpace& space);
	static status_t register_window(X86_32GuestContext& context, AddressSpace& space);
	static status_t unregister_window(X86_32GuestContext& context, AddressSpace& space);
	static status_t set_window_title(X86_32GuestContext& context, AddressSpace& space);
	static status_t show_window(X86_32GuestContext& context, AddressSpace& space);
	static status_t hide_window(X86_32GuestContext& context, AddressSpace& space);
	static status_t move_window(X86_32GuestContext& context, AddressSpace& space);
	static status_t resize_window(X86_32GuestContext& context, AddressSpace& space);
	static status_t destroy_window(X86_32GuestContext& context, AddressSpace& space);

	// Rendering syscalls
	static status_t fill_rect(X86_32GuestContext& context, AddressSpace& space);
	static status_t draw_string(X86_32GuestContext& context, AddressSpace& space);
	static status_t set_color(X86_32GuestContext& context, AddressSpace& space);
	static status_t flush_graphics(X86_32GuestContext& context, AddressSpace& space);

	// Input syscalls
	static status_t get_mouse_position(X86_32GuestContext& context, AddressSpace& space);
	static status_t read_keyboard_input(X86_32GuestContext& context, AddressSpace& space);

	// Window management
	static status_t get_window_frame(X86_32GuestContext& context, AddressSpace& space);
	static status_t set_window_frame(X86_32GuestContext& context, AddressSpace& space);

	// Utility
	static status_t screenshot(X86_32GuestContext& context, AddressSpace& space);
};
