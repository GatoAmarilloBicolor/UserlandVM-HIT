#include "DynamicLinker.h"
#include <stdio.h>
#include <string.h>
#include <libgen.h>

DynamicLinker::DynamicLinker()
{
	// Add default search paths for Haiku 32-bit
	fSearchPaths.push_back("sysroot/haiku32/lib");
	fSearchPaths.push_back("sysroot/haiku32/system/lib");
	fSearchPaths.push_back("sysroot/haiku32/boot/system/lib");
	fSearchPaths.push_back(".");
}

DynamicLinker::~DynamicLinker()
{
	// Clean up loaded libraries
	for (auto &pair : fLibraries) {
		delete pair.second;
	}
}

ElfImage *DynamicLinker::LoadLibrary(const char *path)
{
	if (!path) return NULL;

	// Check if already loaded
	std::string libName = basename((char*)path);
	auto it = fLibraries.find(libName);
	if (it != fLibraries.end()) {
		return it->second;
	}

	// Try to load
	std::string fullPath = ResolveLibPath(path);
	printf("[LINKER] Loading library: %s\n", fullPath.c_str());

	ElfImage *image = ElfImage::Load(fullPath.c_str());
	if (!image) {
		printf("[LINKER] Failed to load: %s\n", fullPath.c_str());
		return NULL;
	}

	fLibraries[libName] = image;
	printf("[LINKER] Loaded %s at %p\n", libName.c_str(), image->GetImageBase());
	return image;
}

bool DynamicLinker::FindSymbol(const char *name, void **address, size_t *size)
{
	if (!name || !address) return false;

	// Search in all loaded libraries
	for (auto &pair : fLibraries) {
		ElfImage *lib = pair.second;
		if (lib && lib->FindSymbol(name, address, size)) {
			printf("[LINKER] Found symbol '%s' at %p\n", name, *address);
			return true;
		}
	}

	printf("[LINKER] Symbol '%s' not found\n", name);
	return false;
}

ElfImage *DynamicLinker::GetLibrary(const char *name)
{
	if (!name) return NULL;

	auto it = fLibraries.find(name);
	if (it != fLibraries.end()) {
		return it->second;
	}
	return NULL;
}

void DynamicLinker::SetSearchPath(const char *path)
{
	if (path) {
		fSearchPaths.insert(fSearchPaths.begin(), path);
	}
}

std::string DynamicLinker::ResolveLibPath(const char *name)
{
	if (!name) return "";

	// If absolute path, return as-is
	if (name[0] == '/') {
		return name;
	}

	// Try each search path
	for (const auto &searchPath : fSearchPaths) {
		std::string fullPath = searchPath + "/" + name;
		FILE *f = fopen(fullPath.c_str(), "rb");
		if (f) {
			fclose(f);
			return fullPath;
		}
	}

	// Fallback: return original name
	return name;
}
