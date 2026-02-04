#pragma once

#include <map>
#include <string>
#include <vector>
#include "Loader.h"

class DynamicLinker {
public:
	struct Symbol {
		const char *name;
		void *address;
		size_t size;
	};

	DynamicLinker();
	~DynamicLinker();

	// Load a dynamic library
	ElfImage *LoadLibrary(const char *path);

	// Find symbol in any loaded library
	bool FindSymbol(const char *name, void **address, size_t *size);

	// Get library by name
	ElfImage *GetLibrary(const char *name);

	// Set library search paths
	void SetSearchPath(const char *path);

private:
	std::map<std::string, ElfImage*> fLibraries;
	std::vector<std::string> fSearchPaths;

	ElfImage *FindInLibraries(const char *name);
	std::string ResolveLibPath(const char *name);
};
