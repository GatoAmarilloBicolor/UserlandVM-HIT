/**
 * @file ETDynIntegration.cpp
 * @brief Implementation that calls CompleteETDynRelocator properly
 */

#include "ETDynIntegration.h"
#include <cstdio>
#include <cstring>
#include <elf.h>

ETDynIntegration::ETDynIntegration(EnhancedDirectAddressSpace* addressSpace)
    : fAddressSpace(addressSpace),
      fRelocator(nullptr),
      fVerboseLogging(false)
{
    if (fAddressSpace) {
        fRelocator = new CompleteETDynRelocator(fAddressSpace);
        fRelocator->SetVerboseLogging(fVerboseLogging);
    }
}

ETDynIntegration::~ETDynIntegration()
{
    if (fRelocator) {
        delete fRelocator;
        fRelocator = nullptr;
    }
}

ETDynIntegration::LoadResult ETDynIntegration::LoadETDynBinary(
    const void* binary_data, size_t binary_size)
{
    LoadResult result = {};
    result.success = false;
    
    if (!fAddressSpace || !fRelocator || !binary_data || binary_size == 0) {
        result.error_message = "Invalid parameters";
        return result;
    }
    
    printf("[ET_DYN_INTEGRATION] Starting ET_DYN binary loading\n");
    printf("[ET_DYN_INTEGRATION] Binary size: %zu bytes\n", binary_size);
    
    // Validate ELF header
    if (!ValidateELFHeader(binary_data, binary_size)) {
        result.error_message = "Invalid ELF header";
        return result;
    }
    
    // Validate ET_DYN type
    if (!ValidateETDynType(binary_data)) {
        result.error_message = "Not an ET_DYN binary";
        return result;
    }
    
    printf("[ET_DYN_INTEGRATION] Valid ET_DYN binary detected\n");
    
    // Calculate memory layout
    MemoryLayout layout = CalculateMemoryLayout(binary_data, binary_size);
    if (!layout.valid) {
        result.error_message = "Failed to calculate memory layout";
        return result;
    }
    
    printf("[ET_DYN_INTEGRATION] Memory layout calculated\n");
    printf("[ET_DYN_INTEGRATION]   Code: 0x%x-0x%x (%u KB)\n",
           layout.code_base, layout.code_base + layout.code_size,
           layout.code_size / 1024);
    printf("[ET_DYN_INTEGRATION]   Data: 0x%x-0x%x (%u KB)\n",
           layout.data_base, layout.data_base + layout.data_size,
           layout.data_size / 1024);
    printf("[ET_DYN_INTEGRATION]   Heap: 0x%x-0x%x (%u KB)\n",
           layout.heap_base, layout.heap_base + layout.heap_size,
           layout.heap_size / 1024);
    
    // CRITICAL: Use CompleteETDynRelocator to load and relocate
    printf("[ET_DYN_INTEGRATION] Calling CompleteETDynRelocator\n");
    
    CompleteETDynRelocator::RelocationResult reloc_result = 
        fRelocator->LoadAndRelocate(binary_data, binary_size, 
                                    &result.load_base, &result.entry_point);
    
    if (!reloc_result.success) {
        result.error_message = "Relocation failed: " + reloc_result.error_message;
        result.failed_relocations = reloc_result.failed_count;
        return result;
    }
    
    result.success = true;
    result.applied_relocations = reloc_result.applied_count;
    result.failed_relocations = reloc_result.failed_count;
    
    printf("[ET_DYN_INTEGRATION] ET_DYN loading completed\n");
    printf("[ET_DYN_INTEGRATION]   Load base: 0x%x\n", result.load_base);
    printf("[ET_DYN_INTEGRATION]   Entry point: 0x%x\n", result.entry_point);
    printf("[ET_DYN_INTEGRATION]   Applied relocations: %u\n", result.applied_relocations);
    printf("[ET_DYN_INTEGRATION]   Failed relocations: %u\n", result.failed_relocations);
    
    // Verify relocations were applied correctly
    if (!VerifyRelocations()) {
        printf("[ET_DYN_INTEGRATION] WARNING: Relocation verification failed\n");
    }
    
    return result;
}

bool ETDynIntegration::ValidateELFHeader(const void* binary_data, size_t binary_size)
{
    if (!binary_data || binary_size < sizeof(Elf32_Ehdr)) {
        printf("[ET_DYN_INTEGRATION] Binary too small for ELF header\n");
        return false;
    }
    
    const uint8_t* data = (const uint8_t*)binary_data;
    
    // Check ELF magic
    if (data[0] != ELFMAG0 || data[1] != ELFMAG1 || 
        data[2] != ELFMAG2 || data[3] != ELFMAG3) {
        printf("[ET_DYN_INTEGRATION] Invalid ELF magic\n");
        return false;
    }
    
    // Check class (32-bit)
    if (data[EI_CLASS] != ELFCLASS32) {
        printf("[ET_DYN_INTEGRATION] Not 32-bit ELF\n");
        return false;
    }
    
    // Check data encoding (little endian)
    if (data[EI_DATA] != ELFDATA2LSB) {
        printf("[ET_DYN_INTEGRATION] Not little-endian ELF\n");
        return false;
    }
    
    // Check version
    if (data[EI_VERSION] != EV_CURRENT) {
        printf("[ET_DYN_INTEGRATION] Invalid ELF version\n");
        return false;
    }
    
    printf("[ET_DYN_INTEGRATION] ELF header validated\n");
    return true;
}

bool ETDynIntegration::ValidateETDynType(const void* binary_data)
{
    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)binary_data;
    
    if (ehdr->e_type != ET_DYN) {
        printf("[ET_DYN_INTEGRATION] Not ET_DYN binary (type: %u)\n", ehdr->e_type);
        return false;
    }
    
    // Check machine (x86)
    if (ehdr->e_machine != EM_386) {
        printf("[ET_DYN_INTEGRATION] Not x86 binary (machine: %u)\n", ehdr->e_machine);
        return false;
    }
    
    printf("[ET_DYN_INTEGRATION] ET_DYN type validated\n");
    return true;
}

ETDynIntegration::MemoryLayout ETDynIntegration::CalculateMemoryLayout(
    const void* binary_data, size_t binary_size)
{
    MemoryLayout layout = {};
    layout.valid = false;
    
    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)binary_data;
    const Elf32_Phdr* phdrs = (const Elf32_Phdr*)((const uint8_t*)binary_data + ehdr->e_phoff);
    
    // Initialize layout
    layout.code_base = ET_DYN_LOAD_BASE;
    layout.code_size = CODE_SEGMENT_SIZE;
    layout.data_base = layout.code_base + layout.code_size;
    layout.data_size = DATA_SEGMENT_SIZE;
    layout.heap_base = layout.data_base + layout.data_size;
    layout.heap_size = HEAP_SEGMENT_SIZE;
    layout.stack_base = 0xC0000000; // 3GB mark
    layout.total_size = layout.stack_base - layout.code_base;
    
    // Find actual segment boundaries
    uint32_t min_vaddr = 0xFFFFFFFF;
    uint32_t max_vaddr = 0;
    
    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const Elf32_Phdr* phdr = &phdrs[i];
        
        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_vaddr < min_vaddr) {
                min_vaddr = phdr->p_vaddr;
            }
            if (phdr->p_vaddr + phdr->p_memsz > max_vaddr) {
                max_vaddr = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }
    
    // Adjust layout based on actual segments
    if (min_vaddr < 0x10000000) { // If segments start before 256MB
        layout.code_base = ET_DYN_LOAD_BASE;
        layout.data_base = layout.code_base + (max_vaddr - min_vaddr);
        layout.heap_base = layout.data_base + DATA_SEGMENT_SIZE;
    }
    
    layout.valid = true;
    
    return layout;
}

status_t ETDynIntegration::AllocateAndMap(uint32_t size, uint32_t* allocated_base)
{
    if (!fAddressSpace || !allocated_base) {
        return B_BAD_VALUE;
    }
    
    // For now, just return a fixed base
    *allocated_base = ET_DYN_LOAD_BASE;
    
    printf("[ET_DYN_INTEGRATION] Allocated %u bytes at 0x%x\n", size, *allocated_base);
    
    return B_OK;
}

bool ETDynIntegration::VerifyRelocations() const
{
    if (!fRelocator) {
        return false;
    }
    
    // Get statistics from relocator
    CompleteETDynRelocator::RelocationStats stats = fRelocator->GetStatistics();
    
    printf("[ET_DYN_INTEGRATION] Relocation verification\n");
    printf("[ET_DYN_INTEGRATION]   Total: %u\n", stats.total_relocations);
    printf("[ET_DYN_INTEGRATION]   Applied: %u\n", stats.applied_relocations);
    printf("[ET_DYN_INTEGRATION]   Failed: %u\n", stats.failed_relocations);
    
    // Check if any relocations failed
    if (stats.failed_relocations > 0) {
        printf("[ET_DYN_INTEGRATION] WARNING: %u relocations failed\n", 
               stats.failed_relocations);
        
        for (const auto& error : stats.errors) {
            printf("[ET_DYN_INTEGRATION] Error: %s\n", error.c_str());
        }
        
        return false;
    }
    
    printf("[ET_DYN_INTEGRATION] All relocations applied successfully\n");
    return true;
}

void ETDynIntegration::DumpLoadInfo(const LoadResult& result)
{
    printf("[ET_DYN_INTEGRATION] === LOAD RESULT ===\n");
    printf("[ET_DYN_INTEGRATION] Success: %s\n", result.success ? "YES" : "NO");
    
    if (result.success) {
        printf("[ET_DYN_INTEGRATION] Load base: 0x%x\n", result.load_base);
        printf("[ET_DYN_INTEGRATION] Entry point: 0x%x\n", result.entry_point);
        printf("[ET_DYN_INTEGRATION] Applied relocations: %u\n", result.applied_relocations);
        printf("[ET_DYN_INTEGRATION] Failed relocations: %u\n", result.failed_relocations);
    } else {
        printf("[ET_DYN_INTEGRATION] Error: %s\n", result.error_message.c_str());
    }
    
    printf("[ET_DYN_INTEGRATION] ======================\n");
}

void ETDynIntegration::ReportError(const std::string& error)
{
    printf("[ET_DYN_INTEGRATION] ERROR: %s\n", error.c_str());
}

ETDynIntegration::LoadResult ETDynIntegration::CreateErrorResult(const std::string& error)
{
    LoadResult result = {};
    result.success = false;
    result.error_message = error;
    result.load_base = 0;
    result.entry_point = 0;
    result.applied_relocations = 0;
    result.failed_relocations = 0;
    
    return result;
}