#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <dlfcn.h>
#include <link.h>
#include "Loader.h"

struct LibraryInfo {
    void* handle = nullptr;
    ElfImage* elf_image = nullptr;
    void* base_address = nullptr;
    size_t size = 0;
    bool loaded = false;
    int reference_count = 0;
};

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
	
	// Add a loaded library directly
	void AddLibrary(const char *name, ElfImage *image);
	
	// Dynamic loading methods
	bool LoadDynamicDependencies(const char *program_path);
	std::vector<std::string> GetDynamicDependencies(const char *program_path);
	bool LoadCriticalLibraries();
	std::string ResolveLibraryPath(const char *name);
	bool IsLibraryLoaded(const char *name) const;
	std::vector<std::string> GetLoadedLibraries() const;

private:
	std::map<std::string, LibraryInfo> fLibraries;
	std::vector<std::string> fSearchPaths;

	// Helper methods
	ElfImage *FindInLibraries(const char *name);
	std::string ResolveLibPath(const char *name);
	std::string GetLibraryName(const char *path) const;
	ElfImage* LoadLibraryELF(const char *path);
	bool FindSymbolInELF(const char *name, void **address, size_t *size);
	size_t GetSymbolSize(void* handle, const char *name);
};
