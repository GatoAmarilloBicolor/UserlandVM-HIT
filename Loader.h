#pragma once

// MUST be first - defines all types before any system headers
#include "PlatformTypes.h"

#include <elf.h>
#include <cstdint>
#include <cstdio>

// AutoDeleter stubs - inline implementation
template<typename T>
class AutoDeleter {
public:
	AutoDeleter() : fObj(nullptr) {}
	AutoDeleter(T* obj) : fObj(obj) {}
	~AutoDeleter() { if (fObj) delete fObj; }
	void SetTo(T* obj) { if (fObj) delete fObj; fObj = obj; }
	T* Get() const { return fObj; }
	T* Detach() { T* obj = fObj; fObj = nullptr; return obj; }
	bool IsSet() const { return fObj != nullptr; }
	operator bool() const { return fObj != nullptr; }
private:
	T* fObj = nullptr;
};

template<typename T>
class ArrayDeleter {
public:
	ArrayDeleter() : fArray(nullptr) {}
	ArrayDeleter(T* array) : fArray(array) {}
	~ArrayDeleter() { if (fArray) delete[] fArray; }
	void SetTo(T* array) { if (fArray) delete[] fArray; fArray = array; }
	T* Get() const { return fArray; }
	T& operator[](size_t i) const { return fArray[i]; }
	bool IsSet() const { return fArray != nullptr; }
private:
	T* fArray = nullptr;
};

class FileCloser {
public:
	FileCloser() : fFile(nullptr) {}
	FileCloser(FILE* f) : fFile(f) {}
	~FileCloser() { if (fFile) fclose(fFile); }
	void SetTo(FILE* f) { if (fFile) fclose(fFile); fFile = f; }
	FILE* Get() const { return fFile; }
	FILE* Detach() { FILE* f = fFile; fFile = nullptr; return f; }
	bool IsSet() const { return fFile != nullptr; }
private:
	FILE* fFile = nullptr;
};

class AreaDeleter {
public:
	AreaDeleter() : fArea(-1) {}
	~AreaDeleter() { if (fArea >= 0) delete_area(fArea); }
	void SetTo(area_id a) { fArea = a; }
	area_id Get() const { return fArea; }
	bool IsSet() const { return fArea >= 0; }
private:
	area_id fArea;
};

template<typename T>
class ObjectDeleter {
public:
	ObjectDeleter() : fObj(nullptr) {}
	ObjectDeleter(T* obj) : fObj(obj) {}
	~ObjectDeleter() { if (fObj) delete fObj; }
	void SetTo(T* obj) { if (fObj) delete fObj; fObj = obj; }
	T* Get() const { return fObj; }
	T* Detach() { T* obj = fObj; fObj = nullptr; return obj; }
	bool IsSet() const { return fObj != nullptr; }
private:
	T* fObj = nullptr;
};

struct Elf32Class {
	static const uint8 identClass = ELFCLASS32;

	typedef uint32 Address;
	typedef int32 PtrDiff;
	typedef Elf32_Ehdr Ehdr;
	typedef Elf32_Phdr Phdr;
	typedef Elf32_Dyn Dyn;
	typedef Elf32_Sym Sym;
	typedef Elf32_Rel Rel;
	typedef Elf32_Rela Rela;
};

struct Elf64Class {
	static const uint8 identClass = ELFCLASS64;

	typedef uint64 Address;
	typedef int64 PtrDiff;
	typedef Elf64_Ehdr Ehdr;
	typedef Elf64_Phdr Phdr;
	typedef Elf64_Dyn Dyn;
	typedef Elf64_Sym Sym;
	typedef Elf64_Rel Rel;
	typedef Elf64_Rela Rela;
};

class ElfImage {
protected:
	FileCloser fFile;
	ArrayDeleter<char> fPath;

	virtual void DoLoad() = 0;

public:
	virtual ~ElfImage() {}
	static ElfImage *Load(const char *path);
	virtual const char *GetArchString() = 0;
	virtual void *GetImageBase() = 0;
	virtual void *GetEntry() = 0;
	virtual bool FindSymbol(const char *name, void **adr, size_t *size) = 0;
	virtual const char *GetPath() = 0;
	virtual bool IsDynamic() = 0;
	
	// New methods for enhanced dynamic loading
	virtual uint32_t GetProgramHeaderCount() = 0;
	virtual uint32_t GetProgramHeaderOffset() = 0;
	virtual uint32_t GetProgramHeaderType(uint32_t index) = 0;
	virtual uint32_t GetProgramHeaderVirtAddr(uint32_t index) = 0;
	virtual uint32_t GetProgramHeaderFileSize(uint32_t index) = 0;
	virtual uint32_t GetProgramHeaderAlign(uint32_t index) = 0;
	virtual uint32_t GetProgramHeaderSize() = 0;
	virtual bool ReadMemory(uint32_t addr, void* buffer, size_t size) = 0;
};

template <typename Class>
class ElfImageImpl: public ElfImage {
private:
	typedef typename Class::Address Address;
	typedef typename Class::PtrDiff PtrDiff;

	typename Class::Ehdr fHeader;
	ArrayDeleter<typename Class::Phdr> fPhdrs;

	AreaDeleter fArea;
	void *fBase{};
	Address fSize{};
	intptr_t fDelta{};  // Use intptr_t to hold full 64-bit offsets

	void *fEntry{};
	typename Class::Dyn *fDynamic{};
	typename Class::Sym *fSymbols{};
	uint32 *fHash{};
	const char *fStrings{};
	bool fIsDynamic{false};

	void *FromVirt(Address virtAdr) {return (void*)((intptr_t)virtAdr + fDelta);}
	Address ToVirt(void *adr) {return (Address)((intptr_t)adr - fDelta);}

	void LoadHeaders();
	void LoadSegments();
	void Relocate();
	void Register();
	void LoadDynamic();

	template<typename Reloc>
	void DoRelocate(Reloc *reloc, Address relocSize);

protected:
	virtual void DoLoad() override;

public:
	virtual ~ElfImageImpl() {}
	const char *GetArchString() override;
	void *GetImageBase() override;
	void *GetEntry() override {return fEntry;}
	bool FindSymbol(const char *name, void **adr, size_t *size) override;
	const char *GetPath() override {return fPath.Get();}
	bool IsDynamic() override {return fIsDynamic;}
	
	// Enhanced methods implementation
	uint32_t GetProgramHeaderCount() override {return fHeader.e_phnum;}
	uint32_t GetProgramHeaderOffset() override {return fHeader.e_phoff;}
	uint32_t GetProgramHeaderType(uint32_t index) override {return fPhdrs[index].p_type;}
	uint32_t GetProgramHeaderVirtAddr(uint32_t index) override {return fPhdrs[index].p_vaddr;}
	uint32_t GetProgramHeaderFileSize(uint32_t index) override {return fPhdrs[index].p_filesz;}
	uint32_t GetProgramHeaderAlign(uint32_t index) override {return fPhdrs[index].p_align;}
	uint32_t GetProgramHeaderSize() override {return fHeader.e_phentsize;}
	bool ReadMemory(uint32_t addr, void* buffer, size_t size) override;
};
