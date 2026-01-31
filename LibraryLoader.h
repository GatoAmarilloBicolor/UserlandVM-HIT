/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include "SymbolResolver.h"
#include <SupportDefs.h>

/**
 * Helper class to load system libraries and populate symbol resolver
 */
class LibraryLoader {
public:
	/**
	 * Load system library (like libroot.so) and extract symbols
	 * Registers found symbols with the provided SymbolResolver
	 * 
	 * @param lib_path Path to library file
	 * @param symbol_resolver SymbolResolver to populate
	 * @param load_base Base address for relocation (typically 0x50000000 or similar)
	 * @return B_OK if successful, error code otherwise
	 */
	static status_t LoadLibrary(const char* lib_path, SymbolResolver& symbol_resolver,
		uint32_t load_base = 0x50000000);

	/**
	 * Load the standard Haiku32 libc (libroot.so)
	 * 
	 * @param sysroot_path Path to sysroot directory (e.g. "./sysroot/haiku32")
	 * @param symbol_resolver SymbolResolver to populate  
	 * @return B_OK if successful, error code otherwise
	 */
	static status_t LoadLibroot(const char* sysroot_path, SymbolResolver& symbol_resolver);

private:
	LibraryLoader() = delete;
	~LibraryLoader() = delete;
};
