#ifndef _IMAGE_DEFS_H
#define _IMAGE_DEFS_H

#include <stdint.h>

// Haiku image definitions for UserlandVM-HIT
// Provides essential constants for dynamic linking

typedef int32_t image_id;
typedef int32_t team_id;

// Image types
#define B_APP_IMAGE		1
#define B_LIBRARY_IMAGE	2
#define B_ADD_ON_IMAGE	3
#define B_SYSTEM_IMAGE	4

// Symbol lookup flags
#define B_SYMBOL_TYPE_ANY		0
#define B_SYMBOL_TYPE_TEXT		1
#define B_SYMBOL_TYPE_DATA		2

// Image types
typedef int32_t image_type;

// Image info structure
struct image_info {
	image_id		id;
	team_id		team;
	image_type		type;
	const char*		name;
	int32_t			image_count;
	void*			dso_handle;
	uintptr_t		text_base;
	uintptr_t		text_size;
	uintptr_t		data_base;
	uintptr_t		data_size;
};

// Image symbol lookup
typedef int32_t (*image_symbol_lookup)(void* opaque, const char* name, 
	void** _symbolLocation, void** _image);

#endif /* _IMAGE_DEFS_H */