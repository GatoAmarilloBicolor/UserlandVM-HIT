/**
 * @file ETDynIntegration.h
 * @brief Integration layer that uses CompleteETDynRelocator properly
 */

#pragma once

#include "UnifiedDefinitionsCorrected.h"
#include "CompleteETDynRelocator.h"
#include "EnhancedDirectAddressSpace.h"
#include <cstdint>
#include <string>

class ETDynIntegration {
public:
    struct LoadResult {
        bool success;
        uint32_t load_base;
        uint32_t entry_point;
        uint32_t applied_relocations;
        uint32_t failed_relocations;
        std::string error_message;
    };

private:
    EnhancedDirectAddressSpace* fAddressSpace;
    CompleteETDynRelocator* fRelocator;
    bool fVerboseLogging;

public:
    ETDynIntegration(EnhancedDirectAddressSpace* addressSpace);
    virtual ~ETDynIntegration();
    
    // Main integration point - THIS IS WHAT MUST BE CALLED
    LoadResult LoadETDynBinary(const void* binary_data, size_t binary_size);
    
    // Memory management helpers
    status_t AllocateAndMap(uint32_t size, uint32_t* allocated_base);
    bool VerifyRelocations() const;
    void DumpLoadInfo(const LoadResult& result);
    
    // Configuration
    void SetVerboseLogging(bool verbose) { 
        fVerboseLogging = verbose; 
        if (fRelocator) fRelocator->SetVerboseLogging(verbose);
    }

private:
    // ELF validation
    bool ValidateELFHeader(const void* binary_data, size_t binary_size);
    bool ValidateETDynType(const void* binary_data);
    
    // Memory layout calculation
    struct MemoryLayout {
        uint32_t code_base;
        uint32_t code_size;
        uint32_t data_base;
        uint32_t data_size;
        uint32_t heap_base;
        uint32_t heap_size;
        uint32_t stack_base;
        uint32_t total_size;
        bool valid;
    };
    
    MemoryLayout CalculateMemoryLayout(const void* binary_data, size_t binary_size);
    
    // Error handling
    void ReportError(const std::string& error);
    LoadResult CreateErrorResult(const std::string& error);
    
    // Constants
    static const uint32_t ET_DYN_LOAD_BASE = 0x08000000;  // 128MB base
    static const uint32_t CODE_SEGMENT_SIZE = 0x01000000;   // 16MB code
    static const uint32_t DATA_SEGMENT_SIZE = 0x01000000;   // 16MB data
    static const uint32_t HEAP_SEGMENT_SIZE = 0x04000000;   // 64MB heap
    static const uint32_t STACK_SEGMENT_SIZE = 0x02000000;  // 32MB stack
};