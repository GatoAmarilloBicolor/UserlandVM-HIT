#include "ELFImage.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

ElfImage::ElfImage()
    : fFile(nullptr)
    , fProgramHeaders(nullptr)
    , fSectionHeaders(nullptr)
    , fStringTable(nullptr)
    , fDynamicStringTable(nullptr)
    , fSymbolTable(nullptr)
    , fDynamicSection(nullptr)
    , fSymbolCount(0)
    , fIsLoaded(false)
{
    memset(&fHeader, 0, sizeof(fHeader));
}

ElfImage::~ElfImage() {
    Cleanup();
}

ElfImage* ElfImage::Load(const char* path) {
    if (!path) return nullptr;
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("[ELF] Failed to open file: %s\n", path);
        return nullptr;
    }
    
    ElfImage* image = new ElfImage();
    image->fFile = file;
    
    // Read ELF header
    if (fread(&image->fHeader, sizeof(image->fHeader), 1, file) != 1) {
        printf("[ELF] Failed to read ELF header\n");
        delete image;
        return nullptr;
    }
    
    // Verify ELF magic
    if (memcmp(image->fHeader.e_ident, ELFMAG, 4) != 0) {
        printf("[ELF] Not an ELF file\n");
        delete image;
        return nullptr;
    }
    
    // Verify 32-bit little-endian
    if (image->fHeader.e_ident[EI_CLASS] != ELFCLASS32 ||
        image->fHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
        printf("[ELF] Unsupported ELF format (not 32-bit little-endian)\n");
        delete image;
        return nullptr;
    }
    
    // Load program headers
    if (image->fHeader.e_phnum > 0) {
        size_t ph_size = image->fHeader.e_phnum * sizeof(Elf32_Phdr);
        image->fProgramHeaders = (Elf32_Phdr*)malloc(ph_size);
        if (!image->fProgramHeaders) {
            printf("[ELF] Failed to allocate program headers\n");
            delete image;
            return nullptr;
        }
        
        fseek(file, image->fHeader.e_phoff, SEEK_SET);
        if (fread(image->fProgramHeaders, ph_size, 1, file) != 1) {
            printf("[ELF] Failed to read program headers\n");
            delete image;
            return nullptr;
        }
    }
    
    // Load PT_INTERP if present
    image->fInterpreterPath = nullptr;
    for (uint32_t i = 0; i < image->fHeader.e_phnum; i++) {
        if (image->fProgramHeaders[i].p_type == PT_INTERP) {
            size_t path_size = image->fProgramHeaders[i].p_filesz;
            image->fInterpreterPath = (char*)malloc(path_size + 1);
            if (!image->fInterpreterPath) {
                printf("[ELF] Failed to allocate interpreter path\n");
                continue;
            }
            
            fseek(file, image->fProgramHeaders[i].p_offset, SEEK_SET);
            if (fread(image->fInterpreterPath, path_size, 1, file) != 1) {
                printf("[ELF] Failed to read interpreter path\n");
                free(image->fInterpreterPath);
                image->fInterpreterPath = nullptr;
                continue;
            }
            
            image->fInterpreterPath[path_size] = '\0';
            printf("[ELF] PT_INTERP loaded: %s\n", image->fInterpreterPath);
            break;
        }
    }
    
    // Load section headers
    if (image->fHeader.e_shnum > 0) {
        size_t sh_size = image->fHeader.e_shnum * sizeof(Elf32_Shdr);
        image->fSectionHeaders = (Elf32_Shdr*)malloc(sh_size);
        if (!image->fSectionHeaders) {
            printf("[ELF] Failed to allocate section headers\n");
            delete image;
            return nullptr;
        }
        
        fseek(file, image->fHeader.e_shoff, SEEK_SET);
        if (fread(image->fSectionHeaders, sh_size, 1, file) != 1) {
            printf("[ELF] Failed to read section headers\n");
            delete image;
            return nullptr;
        }
    }
    
    // Load string tables and symbol table
    image->LoadStringTable();
    image->LoadSymbolTable();
    image->LoadDynamicSection();
    
    image->fIsLoaded = true;
    printf("[ELF] Successfully loaded: %s\n", path);
    
    return image;
}

bool ElfImage::ReadHeaders() {
    return fIsLoaded;
}

bool ElfImage::LoadStringTable() {
    if (!fSectionHeaders) return false;
    
    // Find string table section
    for (uint32_t i = 0; i < fHeader.e_shnum; i++) {
        if (fSectionHeaders[i].sh_type == SHT_STRTAB) {
            size_t size = fSectionHeaders[i].sh_size;
            fStringTable = (char*)malloc(size);
            if (!fStringTable) return false;
            
            fseek(fFile, fSectionHeaders[i].sh_offset, SEEK_SET);
            if (fread(fStringTable, size, 1, fFile) != 1) {
                free(fStringTable);
                fStringTable = nullptr;
                return false;
            }
            return true;
        }
    }
    return false;
}

bool ElfImage::LoadSymbolTable() {
    if (!fSectionHeaders) return false;
    
    // Find symbol table section
    for (uint32_t i = 0; i < fHeader.e_shnum; i++) {
        if (fSectionHeaders[i].sh_type == SHT_SYMTAB) {
            size_t size = fSectionHeaders[i].sh_size;
            fSymbolTable = (Elf32_Sym*)malloc(size);
            if (!fSymbolTable) return false;
            
            fseek(fFile, fSectionHeaders[i].sh_offset, SEEK_SET);
            if (fread(fSymbolTable, size, 1, fFile) != 1) {
                free(fSymbolTable);
                fSymbolTable = nullptr;
                return false;
            }
            
            fSymbolCount = size / sizeof(Elf32_Sym);
            return true;
        }
    }
    return false;
}

bool ElfImage::LoadDynamicSection() {
    if (!fSectionHeaders) return false;
    
    // Find dynamic section
    for (uint32_t i = 0; i < fHeader.e_shnum; i++) {
        if (fSectionHeaders[i].sh_type == SHT_DYNAMIC) {
            size_t size = fSectionHeaders[i].sh_size;
            fDynamicSection = (Elf32_Dyn*)malloc(size);
            if (!fDynamicSection) return false;
            
            fseek(fFile, fSectionHeaders[i].sh_offset, SEEK_SET);
            if (fread(fDynamicSection, size, 1, fFile) != 1) {
                free(fDynamicSection);
                fDynamicSection = nullptr;
                return false;
            }
            
            // Load dynamic string table
            if (fSectionHeaders[i].sh_link < fHeader.e_shnum) {
                Elf32_Shdr& str_tab = fSectionHeaders[fSectionHeaders[i].sh_link];
                if (str_tab.sh_type == SHT_STRTAB) {
                    size_t str_size = str_tab.sh_size;
                    fDynamicStringTable = (char*)malloc(str_size);
                    if (fDynamicStringTable) {
                        fseek(fFile, str_tab.sh_offset, SEEK_SET);
                        fread(fDynamicStringTable, str_size, 1, fFile);
                    }
                }
            }
            
            return true;
        }
    }
    return false;
}

bool ElfImage::IsDynamic() const {
    return fHeader.e_type == ET_DYN;
}

const char* ElfImage::GetArchString() const {
    switch (fHeader.e_machine) {
        case EM_386: return "x86";
        default: return "unknown";
    }
}

uint32_t ElfImage::GetEntry() const {
    return fHeader.e_entry;
}

void* ElfImage::GetBaseAddress() const {
    // For simplicity, return 0 for now
    return nullptr;
}

size_t ElfImage::GetSize() const {
    return 0; // TODO: calculate from headers
}

const Elf32_Ehdr& ElfImage::GetHeader() const {
    return fHeader;
}

const Elf32_Phdr* ElfImage::GetProgramHeaders() const {
    return fProgramHeaders;
}

const Elf32_Shdr* ElfImage::GetSectionHeaders() const {
    return fSectionHeaders;
}

const char* ElfImage::GetStringTable() const {
    return fStringTable;
}

const char* ElfImage::GetDynamicStringTable() const {
    return fDynamicStringTable;
}

const Elf32_Sym* ElfImage::GetSymbolTable() const {
    return fSymbolTable;
}

const Elf32_Dyn* ElfImage::GetDynamicSection() const {
    return fDynamicSection;
}

uint32_t ElfImage::GetSymbolCount() const {
    return fSymbolCount;
}

const char* ElfImage::GetSymbolName(uint32_t index) const {
    if (!fStringTable || index >= fSymbolCount) {
        return nullptr;
    }
    
    uint32_t name_offset = fSymbolTable[index].st_name;
    if (name_offset == 0) {
        return "";
    }
    
    return fStringTable + name_offset;
}

void ElfImage::Cleanup() {
    if (fFile) {
        fclose(fFile);
        fFile = nullptr;
    }
    
    if (fProgramHeaders) {
        free(fProgramHeaders);
        fProgramHeaders = nullptr;
    }
    
    if (fSectionHeaders) {
        free(fSectionHeaders);
        fSectionHeaders = nullptr;
    }
    
    if (fStringTable) {
        free(fStringTable);
        fStringTable = nullptr;
    }
    
    if (fDynamicStringTable) {
        free(fDynamicStringTable);
        fDynamicStringTable = nullptr;
    }
    
    if (fSymbolTable) {
        free(fSymbolTable);
        fSymbolTable = nullptr;
    }
    
    if (fDynamicSection) {
        free(fDynamicSection);
        fDynamicSection = nullptr;
    }
    
    if (fInterpreterPath) {
        free(fInterpreterPath);
        fInterpreterPath = nullptr;
    }
    
    fIsLoaded = false;
}

// PT_INTERP support implementation
bool ElfImage::HasInterpreter() const {
    return fInterpreterPath != nullptr;
}

const char* ElfImage::GetInterpreterPath() const {
    return fInterpreterPath;
}