#pragma once

// MUST be first - defines all types before any system headers
#include "PlatformTypes.h"

#include <elf.h>
#include <cstdint>
#include <memory>
#include <string>

// AutoDeleter implementations - simplified for compatibility
template<typename T>
class AutoDeleter {
public:
    AutoDeleter() : fObj(nullptr) {}
    AutoDeleter(T* obj) : fObj(obj) {}
    ~AutoDeleter() { if (fObj) delete fObj; }
    
    void SetTo(T* obj) { 
        if (fObj) delete fObj; 
        fObj = obj; 
    }
    
    T* Get() const { return fObj; }
    T* Release() { 
        T* obj = fObj; 
        fObj = nullptr; 
        return obj; 
    }
    
    T& operator*() const { return *fObj; }
    T* operator->() const { return fObj; }
    operator bool() const { return fObj != nullptr; }

private:
    T* fObj;
};

template<typename T>
class ArrayDeleter {
public:
    ArrayDeleter() : fArray(nullptr) {}
    ArrayDeleter(T* array) : fArray(array) {}
    ~ArrayDeleter() { if (fArray) delete[] fArray; }
    
    void SetTo(T* array) { 
        if (fArray) delete[] fArray; 
        fArray = array; 
    }
    
    T* Get() const { return fArray; }
    T* Release() { 
        T* array = fArray; 
        fArray = nullptr; 
        return array; 
    }
    
    T& operator[](size_t index) const { return fArray[index]; }
    operator bool() const { return fArray != nullptr; }

private:
    T* fArray;
};

// File handle RAII
class FileCloser {
public:
    FileCloser() : fFile(nullptr) {}
    FileCloser(FILE* file) : fFile(file) {}
    ~FileCloser() { if (fFile) fclose(fFile); }
    
    void SetTo(FILE* file) { 
        if (fFile) fclose(fFile); 
        fFile = file; 
    }
    
    FILE* Get() const { return fFile; }
    FILE* Release() { 
        FILE* file = fFile; 
        fFile = nullptr; 
        return file; 
    }
    
    operator bool() const { return fFile != nullptr; }

private:
    FILE* fFile;
};

struct Elf32Class {
	static const uint8_t identClass = ELFCLASS32;

	typedef uint32_t Address;
	typedef int32_t PtrDiff;
	typedef Elf32_Ehdr Ehdr;
	typedef Elf32_Phdr Phdr;
	typedef Elf32_Dyn Dyn;
	typedef Elf32_Sym Sym;
	typedef Elf32_Rel Rel;
	typedef Elf32_Rela Rela;

	static constexpr uint8_t kIdentClass = ELFCLASS32;
	static constexpr uint8_t kIdentData = ELFDATA2LSB;
	static constexpr uint16_t kType = ET_EXEC;
	static constexpr uint32_t kMachine = EM_386;
	static constexpr uint32_t kVersion = EV_CURRENT;

	static constexpr uint32_t kDynamicNull = DT_NULL;
	static constexpr uint32_t kDynamicNeeded = DT_NEEDED;
	static constexpr uint32_t kDynamicPltRelSz = DT_PLTRELSZ;
	static constexpr uint32_t kDynamicPltRel = DT_PLTREL;
	static constexpr uint32_t kDynamicJmpRel = DT_JMPREL;
	static constexpr uint32_t kDynamicSymTab = DT_SYMTAB;
	static constexpr uint32_t kDynamicStrTab = DT_STRTAB;

	// Constants for relocations - use numeric values directly
	static constexpr uint32_t kRelTypeJumpSlot = 7;    // R_386_JUMP_SLOT
	static constexpr uint32_t kRelTypeGlobalData = 6;   // R_386_GLOB_DAT
	static constexpr uint32_t kRelTypeRelative = 8;      // R_386_RELATIVE
	static constexpr uint32_t kRelType32 = 2;           // R_386_32
};

using Elf32AutoDeleter = AutoDeleter<Elf32Class>;
using FileAutoDeleter = FileCloser;