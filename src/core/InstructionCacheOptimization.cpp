/*
 * InstructionCacheOptimization.cpp - Implementation with advanced caching
 */

#include "InstructionCacheOptimization.h"
#include <cstdio>
#include <algorithm>

InstructionCacheOptimization* g_instruction_cache = nullptr;

InstructionCacheOptimization::InstructionCacheOptimization() 
    : hit_count(0), miss_count(0) {
    
    // Initialize cache entries to invalid state
    for (auto& entry : cache_entries) {
        entry.address = INVALID_TARGET;
        entry.opcode = 0;
        entry.arg1 = 0;
        entry.arg2 = 0;
        entry.instruction_len = 0;
        entry.flags = 0;
        entry.jump_target = INVALID_TARGET;
        entry.access_count = 0;
    }
    
    for (auto& target : jump_targets) {
        target = INVALID_TARGET;
    }
    
    printf("[CACHE] Advanced instruction cache initialized (%zu entries)\n", CACHE_SIZE);
}

InstructionCacheOptimization::CacheEntry* InstructionCacheOptimization::Get(uint32_t address) {
    uint32_t index = HashAddress(address);
    CacheEntry& entry = cache_entries[index];
    
    if (UNLIKELY(entry.address != address)) {
        miss_count++;
        LOG_VERBOSE("[CACHE] Miss: 0x%08x (index: %u)\n", address, index);
        return nullptr;
    }
    
    hit_count++;
    entry.access_count++;
    
    // Update hot path flag
    if (entry.access_count >= hot_path_threshold) {
        entry.flags |= FLAG_HOT_PATH;
    }
    
    LOG_VERBOSE("[CACHE] Hit: 0x%08x (count: %u)\n", address, entry.access_count);
    
    return &entry;
}

void InstructionCacheOptimization::Put(uint32_t address, uint32_t opcode, uint32_t arg1, uint32_t arg2) {
    uint32_t index = HashAddress(address);
    CacheEntry& entry = cache_entries[index];
    
    // Check if this is a frequently accessed location
    bool is_hot = IsInHotPath(address);
    
    // Determine instruction flags
    uint16_t flags = 0;
    switch (opcode & 0xFF) {
        case 0xE8: case 0xFF: flags |= FLAG_IS_CALL; break;  // CALL
        case 0xC3: case 0xC2: case 0xCA: flags |= FLAG_IS_RET; break;  // RET
        case 0x74: case 0x75: case 0x7E: case 0x7F:  // Conditional jumps
        case 0xEB: case 0xE9: case 0xEA: flags |= FLAG_IS_JUMP; break;
        case 0x88: case 0x89: flags |= FLAG_IS_MOV; break;  // MOV
        case 0x00: case 0x01: case 0x02: case 0x03:  // ADD
        case 0x04: case 0x05: case 0x28: case 0x29: flags |= FLAG_IS_ADD; break;
    }
    
    if (is_hot) flags |= FLAG_HOT_PATH;
    
    // Only cache if it's a hot location or replaces cold entry
    if (is_hot || entry.access_count < hot_path_threshold) {
        entry.address = address;
        entry.opcode = opcode;
        entry.arg1 = arg1;
        entry.arg2 = arg2;
        entry.instruction_len = GetInstructionLength(opcode);
        entry.flags = flags;
        entry.access_count = 1;  // Reset for new entry
        
        LOG_VERBOSE("[CACHE] Put: 0x%08x (opcode: 0x%02x, flags: 0x%04x)\n", 
                   address, opcode & 0xFF, flags);
    }
}

void InstructionCacheOptimization::Invalidate(uint32_t address) {
    uint32_t index = HashAddress(address);
    CacheEntry& entry = cache_entries[index];
    
    if (entry.address == address) {
        entry.address = INVALID_TARGET;
        LOG_VERBOSE("[CACHE] Invalidate: 0x%08x\n", address);
    }
}

void InstructionCacheOptimization::InvalidateRange(uint32_t start, uint32_t end) {
    for (uint32_t addr = start; addr <= end; addr += 16) {  // Assume 16-byte alignment
        Invalidate(addr);
    }
    
    LOG_VERBOSE("[CACHE] Invalidated range: 0x%08x - 0x%08x\n", start, end);
}

uint32_t InstructionCacheOptimization::PredictNextInstruction(uint32_t current_address) {
    if (!prediction_enabled) return INVALID_TARGET;
    
    // Look up current address in cache
    CacheEntry* entry = Get(current_address);
    if (!entry) return INVALID_TARGET;
    
    // If this is a jump instruction, predict the target
    if (entry->flags & FLAG_IS_JUMP) {
        return entry->jump_target;
    }
    
    // For linear code, predict next sequential instruction
    return current_address + entry->instruction_len;
}

void InstructionCacheOptimization::RecordJumpTarget(uint32_t from_addr, uint32_t to_addr) {
    uint32_t index = JumpHash(from_addr, to_addr);
    jump_targets[index] = to_addr;
    
    LOG_VERBOSE("[CACHE] Jump recorded: 0x%08x -> 0x%08x\n", from_addr, to_addr);
}

bool InstructionCacheOptimization::IsInHotPath(uint32_t address) {
    CacheEntry* entry = Get(address);
    return entry && (entry->flags & FLAG_HOT_PATH);
}

uint32_t InstructionCacheOptimization::HashAddress(uint32_t address) const {
    // Better hash function to reduce collisions
    address ^= address >> 16;
    address *= 0x85ebca6b;
    address ^= address >> 13;
    address *= 0xc2b2ae35;
    address ^= address >> 16;
    
    return address & CACHE_INDEX_MASK;
}

uint32_t InstructionCacheOptimization::JumpHash(uint32_t from_addr, uint32_t to_addr) const {
    // Hash jump targets for prediction
    uint32_t combined = from_addr ^ to_addr;
    return HashAddress(combined);
}

uint16_t InstructionCacheOptimization::GetInstructionLength(uint32_t opcode) const {
    // Fast instruction length lookup
    uint8_t prefix = opcode >> 24;  // Prefix byte
    uint8_t primary = (opcode >> 16) & 0xFF;  // Primary opcode
    
    // Common instruction lengths
    switch (primary) {
        case 0x50: case 0x51: case 0x52: case 0x53:  // PUSH reg
        case 0x54: case 0x55: case 0x56: case 0x57:
            return 1;
        case 0x58: case 0x59: case 0x5A: case 0x5B:  // POP reg
        case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            return 1;
        case 0x88: case 0x89:                      // MOV r/m, r
        case 0x8A: case 0x8B:                      // MOV r, r/m
            return 2;
        case 0xB8: case 0xB9: case 0xBA: case 0xBB:  // MOV r, imm32
        case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            return 5;
        case 0xC3:                                 // RET
            return 1;
        case 0xE8:                                 // CALL rel32
            return 5;
        case 0xE9:                                 // JMP rel32
            return 5;
        case 0xEB:                                 // JMP rel8
            return 2;
        default:
            return 1;  // Default, interpreter will handle correctly
    }
}

void InstructionCacheOptimization::UpdateAccessPattern(CacheEntry& entry) {
    entry.access_count++;
    
    // Check if this becomes a hot path
    if (entry.access_count >= hot_path_threshold) {
        entry.flags |= FLAG_HOT_PATH;
    }
}

void InstructionCacheOptimization::DumpStats() const {
    printf("\n=== INSTRUCTION CACHE STATISTICS ===\n");
    printf("Cache entries: %zu\n", CACHE_SIZE);
    printf("Hit count: %u\n", hit_count);
    printf("Miss count: %u\n", miss_count);
    printf("Hit rate: %.2f%%\n", GetHitRate());
    printf("Hot path threshold: %u\n", hot_path_threshold);
    printf("Prediction enabled: %s\n", prediction_enabled ? "YES" : "NO");
    
    // Cache utilization
    uint32_t valid_entries = 0;
    uint32_t hot_entries = 0;
    
    for (const auto& entry : cache_entries) {
        if (entry.address != INVALID_TARGET) {
            valid_entries++;
            if (entry.flags & FLAG_HOT_PATH) {
                hot_entries++;
            }
        }
    }
    
    printf("Valid entries: %u/%zu (%.1f%%)\n", 
           valid_entries, CACHE_SIZE, 
           (double)valid_entries * 100.0 / CACHE_SIZE);
    printf("Hot path entries: %u (%.1f%% of valid)\n", 
           hot_entries, valid_entries > 0 ? (double)hot_entries * 100.0 / valid_entries : 0.0);
    printf("================================\n\n");
}

void InstructionCacheOptimization::SetHotPathThreshold(uint32_t threshold) {
    hot_path_threshold = threshold;
    LOG_VERBOSE("[CACHE] Hot path threshold set to %u\n", threshold);
}

void InstructionCacheOptimization::EnablePrediction(bool enabled) {
    prediction_enabled = enabled;
    printf("[CACHE] Jump prediction %s\n", enabled ? "ENABLED" : "DISABLED");
}
