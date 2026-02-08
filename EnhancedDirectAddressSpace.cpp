/**
 * @file EnhancedDirectAddressSpace.cpp
 * @brief Implementation of enhanced 4GB address space with ET_DYN support
 */

#include "EnhancedDirectAddressSpace.h"
#include "UnifiedDefinitionsCorrected.h"
#include <sys/mman.h>

EnhancedDirectAddressSpace::EnhancedDirectAddressSpace()
    : fMemory(nullptr),
      fGuestSize(0),
      fGuestBaseAddress(0),
      fHeapBase(0),
      fHeapSize(0),
      fHeapNext(0),
      fStackBase(0),
      fStackSize(0),
      fLoadBias(0),
      fETDynLoaded(false)
{
    fRegions.reserve(64); // Pre-allocate for efficiency
}

EnhancedDirectAddressSpace::~EnhancedDirectAddressSpace()
{
    FreeHostMemory();
}

status_t EnhancedDirectAddressSpace::Init(size_t size)
{
    if (fMemory) {
        return B_BAD_VALUE;
    }
    
    // Align to page size
    size_t pageSize = 4096;
    size = (size + pageSize - 1) & ~(pageSize - 1);
    
    // Allocate host memory backing
    status_t result = AllocateHostMemory(size);
    if (result != B_OK) {
        return result;
    }
    
    fGuestSize = size;
    fGuestBaseAddress = (uintptr_t)fMemory;
    
    printf("[ENHANCED_ADDRESS_SPACE] Initialized %zu bytes (0x%zx) of guest memory\n", size, size);
    printf("[ENHANCED_ADDRESS_SPACE] Host pointer: %p\n", fMemory);
    
    // Set up standard memory regions
    // Code region (traditional executable base)
    AddRegion(STANDARD_CODE_BASE, 0x08000000, MEMORY_TYPE_CODE, "standard_code");
    
    // ET_DYN region (PIE binaries)
    AddRegion(ET_DYN_BASE, 0x08000000, MEMORY_TYPE_CODE, "et_dyn_code");
    
    // Heap region
    status_t heap_result = AllocateHeap(&fHeapBase, 0x10000000); // 256MB initial heap
    if (heap_result != B_OK) {
        printf("[ENHANCED_ADDRESS_SPACE] Failed to initialize heap: %d\n", heap_result);
        return heap_result;
    }
    
    // Stack region
    status_t stack_result = AllocateStack(&fStackBase, STACK_SIZE);
    if (stack_result != B_OK) {
        printf("[ENHANCED_ADDRESS_SPACE] Failed to initialize stack: %d\n", stack_result);
        return stack_result;
    }
    
    printf("[ENHANCED_ADDRESS_SPACE] Memory map initialized\n");
    DumpMemoryMap();
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::AllocateHostMemory(size_t size)
{
    // Use mmap for large allocations (4GB)
    fMemory = (uint8_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (fMemory == MAP_FAILED) {
        printf("[ENHANCED_ADDRESS_SPACE] Failed to mmap %zu bytes: %s\n", 
               size, strerror(errno));
        return B_NO_MEMORY;
    }
    
    // Mark memory as inaccessible initially (will be enabled per region)
    mprotect(fMemory, size, PROT_NONE);
    
    printf("[ENHANCED_ADDRESS_SPACE] Allocated %zu bytes at %p\n", size, fMemory);
    return B_OK;
}

void EnhancedDirectAddressSpace::FreeHostMemory()
{
    if (fMemory) {
        munmap(fMemory, fGuestSize);
        fMemory = nullptr;
    }
}

status_t EnhancedDirectAddressSpace::Read(uintptr_t guestAddress, void* buffer, size_t size)
{
    if (!fMemory || !buffer || size == 0) {
        return B_BAD_VALUE;
    }
    
    if (!IsValidAddress(guestAddress) || !IsValidAddress(guestAddress + size - 1)) {
        printf("[ENHANCED_ADDRESS_SPACE] Invalid read address: 0x%lx (size: %zu)\n", 
               guestAddress, size);
        return B_BAD_ADDRESS;
    }
    
    // Check read permissions
    if (!CheckMemoryAccess(guestAddress, size, false)) {
        printf("[ENHANCED_ADDRESS_SPACE] Read access denied: 0x%lx (size: %zu)\n", 
               guestAddress, size);
        return B_PERMISSION_DENIED;
    }
    
    uint8_t* src = fMemory + guestAddress;
    memcpy(buffer, src, size);
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::Write(uintptr_t guestAddress, const void* buffer, size_t size)
{
    if (!fMemory || !buffer || size == 0) {
        return B_BAD_VALUE;
    }
    
    if (!IsValidAddress(guestAddress) || !IsValidAddress(guestAddress + size - 1)) {
        printf("[ENHANCED_ADDRESS_SPACE] Invalid write address: 0x%lx (size: %zu)\n", 
               guestAddress, size);
        return B_BAD_ADDRESS;
    }
    
    // Check write permissions
    if (!CheckMemoryAccess(guestAddress, size, true)) {
        printf("[ENHANCED_ADDRESS_SPACE] Write access denied: 0x%lx (size: %zu)\n", 
               guestAddress, size);
        return B_PERMISSION_DENIED;
    }
    
    uint8_t* dst = fMemory + guestAddress;
    memcpy(dst, buffer, size);
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::ReadString(uintptr_t guestAddress, char* buffer, size_t bufferSize)
{
    if (!fMemory || !buffer || bufferSize == 0) {
        return B_BAD_VALUE;
    }
    
    if (!IsValidAddress(guestAddress)) {
        return B_BAD_ADDRESS;
    }
    
    // Read until null terminator or buffer full
    size_t i = 0;
    while (i < bufferSize - 1 && IsValidAddress(guestAddress + i)) {
        buffer[i] = fMemory[guestAddress + i];
        if (buffer[i] == '\0') {
            return B_OK;
        }
        i++;
    }
    
    buffer[bufferSize - 1] = '\0';
    return (i < bufferSize - 1) ? B_OK : B_BUFFER_OVERFLOW;
}

status_t EnhancedDirectAddressSpace::RegisterMapping(uintptr_t guest_vaddr, uintptr_t guest_offset, 
                                                   size_t size, MemoryType type, const std::string& name)
{
    printf("[ENHANCED_ADDRESS_SPACE] Registering mapping: 0x%lx -> 0x%lx (size: %zu, type: %d, name: %s)\n",
           guest_vaddr, guest_offset, size, (int)type, name.c_str());
    
    // Add region
    status_t result = AddRegion(guest_vaddr, size, type, name);
    if (result != B_OK) {
        return result;
    }
    
    // Enable memory protection for this region
    uint32_t protection = PROT_READ | PROT_WRITE;
    if (type == MEMORY_TYPE_CODE) {
        protection |= PROT_EXEC;
    }
    
    ProtectMemory(guest_vaddr, size, protection);
    
    return B_OK;
}

uintptr_t EnhancedDirectAddressSpace::TranslateAddress(uintptr_t guest_vaddr) const
{
    if (!IsValidAddress(guest_vaddr)) {
        return 0;
    }
    
    return fGuestBaseAddress + guest_vaddr;
}

status_t EnhancedDirectAddressSpace::LoadETDynBinary(const void* binary_data, size_t binary_size, 
                                                   uint32_t* load_base, uint32_t* entry_point)
{
    printf("[ENHANCED_ADDRESS_SPACE] Loading ET_DYN binary (size: %zu)\n", binary_size);
    
    if (!binary_data || !load_base || !entry_point) {
        return B_BAD_VALUE;
    }
    
    // Parse ELF header
    const uint8_t* data = (const uint8_t*)binary_data;
    if (binary_size < sizeof(Elf32_Ehdr)) {
        printf("[ENHANCED_ADDRESS_SPACE] Binary too small for ELF header\n");
        return B_BAD_DATA;
    }
    
    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)data;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        printf("[ENHANCED_ADDRESS_SPACE] Invalid ELF magic\n");
        return B_BAD_DATA;
    }
    
    if (ehdr->e_type != ET_DYN) {
        printf("[ENHANCED_ADDRESS_SPACE] Not an ET_DYN binary (type: %d)\n", ehdr->e_type);
        return B_BAD_TYPE;
    }
    
    // Calculate load bias and base address
    fLoadBias = ET_DYN_BASE;
    uint32_t base_address = fLoadBias;
    
    printf("[ENHANCED_ADDRESS_SPACE] ET_DYN load bias: 0x%x\n", fLoadBias);
    
    // Load program headers
    const Elf32_Phdr* phdrs = (const Elf32_Phdr*)(data + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        const Elf32_Phdr* phdr = &phdrs[i];
        
        if (phdr->p_type == PT_LOAD) {
            uint32_t vaddr = base_address + phdr->p_vaddr;
            uint32_t mem_size = phdr->p_memsz;
            uint32_t file_size = phdr->p_filesz;
            
            printf("[ENHANCED_ADDRESS_SPACE] Loading segment %d: vaddr=0x%x, mem_size=0x%x, file_size=0x%x\n",
                   i, vaddr, mem_size, file_size);
            
            // Register mapping for this segment
            RegisterMapping(vaddr, phdr->p_offset, mem_size, MEMORY_TYPE_CODE, "et_dyn_segment");
            
            // Copy segment data
            if (file_size > 0) {
                status_t result = Write(vaddr, data + phdr->p_offset, file_size);
                if (result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to write segment %d: %d\n", i, result);
                    return result;
                }
            }
            
            // Zero-fill remaining memory
            if (mem_size > file_size) {
                std::vector<uint8_t> zeros(mem_size - file_size, 0);
                status_t result = Write(vaddr + file_size, zeros.data(), zeros.size());
                if (result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to zero-fill segment %d: %d\n", i, result);
                    return result;
                }
            }
        }
    }
    
    *load_base = base_address;
    *entry_point = base_address + ehdr->e_entry;
    
    fETDynLoaded = true;
    
    printf("[ENHANCED_ADDRESS_SPACE] ET_DYN binary loaded successfully\n");
    printf("[ENHANCED_ADDRESS_SPACE] Load base: 0x%x, Entry point: 0x%x\n", *load_base, *entry_point);
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::ApplyRelocations(uint32_t load_base, const void* relocations, 
                                                   size_t rel_count)
{
    printf("[ENHANCED_ADDRESS_SPACE] Applying %zu relocations at base 0x%x\n", rel_count, load_base);
    
    if (!fETDynLoaded) {
        printf("[ENHANCED_ADDRESS_SPACE] No ET_DYN binary loaded\n");
        return B_BAD_VALUE;
    }
    
        const Elf32_Rel* rels = (const Elf32_Rel*)relocations;
    
    for (size_t i = 0; i < rel_count; i++) {
        const Elf32_Rel* rel = &rels[i];
        uint32_t reloc_type = ELF32_R_TYPE(rel->r_info);
        uint32_t reloc_offset = load_base + rel->r_offset;
        
        printf("[ENHANCED_ADDRESS_SPACE] Processing relocation %zu: type=%d, offset=0x%x\n",
               i, reloc_type, reloc_offset);
        
        switch (reloc_type) {
            case R_386_RELATIVE: {
                 // Base-relative relocation
                int32_t addend = 0;
                uint32_t value = load_base;
                
                printf("[ENHANCED_ADDRESS_SPACE] R_386_RELATIVE: offset=0x%x, addend=0x%x, value=0x%x\n",
                       reloc_offset, addend, value);
                
                status_t result = Write(reloc_offset, &value, sizeof(value));
                if (result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to apply R_386_RELATIVE: %d\n", result);
                    return result;
                }
                break;
            }
            
            case R_386_32: {
                // Absolute relocation (symbol + addend)
                uint32_t current_value;
                status_t read_result = Read(reloc_offset, &current_value, sizeof(current_value));
                if (read_result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to read R_386_32 value: %d\n", read_result);
                    return read_result;
                }
                
                 uint32_t new_value = load_base + current_value + rel->r_addend - reloc_offset;
                
                printf("[ENHANCED_ADDRESS_SPACE] R_386_32: offset=0x%x, current=0x%x, new=0x%x\n",
                       reloc_offset, current_value, new_value);
                
                status_t write_result = Write(reloc_offset, &new_value, sizeof(new_value));
                if (write_result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to apply R_386_32: %d\n", write_result);
                    return write_result;
                }
                break;
            }
            
            case R_386_PC32: {
                // PC-relative relocation (symbol + addend - offset)
                uint32_t current_value;
                status_t read_result = Read(reloc_offset, &current_value, sizeof(current_value));
                if (read_result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to read R_386_PC32 value: %d\n", read_result);
                    return read_result;
                }
                
                uint32_t new_value = load_base + current_value + rel->r_addend - reloc_offset;
                
                printf("[ENHANCED_ADDRESS_SPACE] R_386_PC32: offset=0x%x, current=0x%x, new=0x%x\n",
                       reloc_offset, current_value, new_value);
                
                status_t write_result = Write(reloc_offset, &new_value, sizeof(new_value));
                if (write_result != B_OK) {
                    printf("[ENHANCED_ADDRESS_SPACE] Failed to apply R_386_PC32: %d\n", write_result);
                    return write_result;
                }
                break;
            }
            
            case R_386_NONE:
                // No relocation needed
                break;
                
            default:
                printf("[ENHANCED_ADDRESS_SPACE] Unsupported relocation type: %d\n", reloc_type);
                break;
        }
    }
    
    printf("[ENHANCED_ADDRESS_SPACE] Applied %zu relocations successfully\n", rel_count);
    return B_OK;
}

status_t EnhancedDirectAddressSpace::AllocateHeap(uint32_t* heap_base, size_t initial_size)
{
    if (!heap_base || initial_size == 0) {
        return B_BAD_VALUE;
    }
    
    fHeapBase = HEAP_BASE;
    fHeapSize = initial_size;
    fHeapNext = fHeapBase;
    
    // Register heap region
    status_t result = AddRegion(fHeapBase, fHeapSize, MEMORY_TYPE_HEAP, "heap");
    if (result != B_OK) {
        return result;
    }
    
    // Enable memory protection
    ProtectMemory(fHeapBase, fHeapSize, PROT_READ | PROT_WRITE);
    
    *heap_base = fHeapBase;
    
    printf("[ENHANCED_ADDRESS_SPACE] Heap allocated: base=0x%x, size=0x%zx\n", 
           fHeapBase, initial_size);
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::ExpandHeap(size_t additional_size)
{
    if (additional_size == 0) {
        return B_BAD_VALUE;
    }
    
    size_t new_size = fHeapSize + additional_size;
    size_t max_heap_size = STACK_BASE - HEAP_BASE; // Don't overlap with stack
    
    if (new_size > max_heap_size) {
        printf("[ENHANCED_ADDRESS_SPACE] Heap expansion would exceed maximum size\n");
        return B_NO_MEMORY;
    }
    
    // Expand heap region
    status_t result = AddRegion(fHeapBase, new_size, MEMORY_TYPE_HEAP, "heap_expanded");
    if (result != B_OK) {
        return result;
    }
    
    fHeapSize = new_size;
    
    printf("[ENHANCED_ADDRESS_SPACE] Heap expanded to: 0x%zx bytes\n", fHeapSize);
    return B_OK;
}

status_t EnhancedDirectAddressSpace::AllocateStack(uint32_t* stack_base, size_t stack_size)
{
    if (!stack_base || stack_size == 0) {
        return B_BAD_VALUE;
    }
    
    fStackSize = stack_size;
    fStackBase = STACK_BASE; // Stack grows down
    
    // Register stack region
    status_t result = AddRegion(fStackBase - fStackSize, fStackSize, MEMORY_TYPE_STACK, "stack");
    if (result != B_OK) {
        return result;
    }
    
    // Enable memory protection
    ProtectMemory(fStackBase - fStackSize, fStackSize, PROT_READ | PROT_WRITE);
    
    *stack_base = fStackBase;
    
    printf("[ENHANCED_ADDRESS_SPACE] Stack allocated: top=0x%x, size=0x%zx\n", 
           fStackBase, stack_size);
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::ProtectMemory(uintptr_t address, size_t size, uint32_t protection)
{
    if (!fMemory || !IsValidAddress(address) || size == 0) {
        return B_BAD_VALUE;
    }
    
    uint8_t* mem_ptr = fMemory + address;
    int prot = 0;
    
    if (protection & PROT_READ) prot |= PROT_READ;
    if (protection & PROT_WRITE) prot |= PROT_WRITE;
    if (protection & PROT_EXEC) prot |= PROT_EXEC;
    
    int result = mprotect(mem_ptr, size, prot);
    if (result != 0) {
        printf("[ENHANCED_ADDRESS_SPACE] mprotect failed: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    return B_OK;
}

status_t EnhancedDirectAddressSpace::CheckMemoryAccess(uintptr_t address, size_t size, bool is_write) const
{
    const MemoryRegion* region = FindRegion(address);
    if (!region) {
        printf("[ENHANCED_ADDRESS_SPACE] No region for address 0x%lx\n", address);
        return B_BAD_ADDRESS;
    }
    
    if (address + size > region->end) {
        printf("[ENHANCED_ADDRESS_SPACE] Access beyond region bounds\n");
        return B_BAD_ADDRESS;
    }
    
    if (is_write && !(region->protection & PROT_WRITE)) {
        printf("[ENHANCED_ADDRESS_SPACE] Write access denied on read-only region\n");
        return B_PERMISSION_DENIED;
    }
    
    return B_OK;
}

void EnhancedDirectAddressSpace::DumpMemoryMap()
{
    printf("[ENHANCED_ADDRESS_SPACE] Memory Map:\n");
    printf("[ENHANCED_ADDRESS_SPACE] Total size: 0x%zx (%zu MB)\n", fGuestSize, fGuestSize / (1024 * 1024));
    printf("[ENHANCED_ADDRESS_SPACE] Regions:\n");
    
    for (const auto& region : fRegions) {
        const char* type_str = "unknown";
        switch (region.type) {
            case MEMORY_TYPE_CODE: type_str = "CODE"; break;
            case MEMORY_TYPE_DATA: type_str = "DATA"; break;
            case MEMORY_TYPE_HEAP: type_str = "HEAP"; break;
            case MEMORY_TYPE_STACK: type_str = "STACK"; break;
            case MEMORY_TYPE_MMAP: type_str = "MMAP"; break;
            case MEMORY_TYPE_SHARED: type_str = "SHARED"; break;
        }
        
        const char* prot_str = "---";
        if (region.protection & PROT_READ) prot_str = "r--";
        if (region.protection & PROT_WRITE) prot_str = (region.protection & PROT_READ) ? "rw-" : "-w-";
        if (region.protection & PROT_EXEC) {
            if (region.protection & PROT_READ) prot_str = "r-x";
            else if (region.protection & PROT_WRITE) prot_str = "-wx";
            else prot_str = "--x";
        }
        
        printf("[ENHANCED_ADDRESS_SPACE]   0x%08x-0x%08x (%8zu KB) %s %s %s\n",
               region.start, region.end, region.size / 1024, type_str, prot_str, region.name.c_str());
    }
}

bool EnhancedDirectAddressSpace::IsValidAddress(uintptr_t address) const
{
    return address < fGuestSize && fMemory != nullptr;
}

uint8_t* EnhancedDirectAddressSpace::GetHostPointer(uintptr_t guest_address)
{
    if (!IsValidAddress(guest_address)) {
        return nullptr;
    }
    
    return fMemory + guest_address;
}

status_t EnhancedDirectAddressSpace::AddRegion(uint32_t start, uint32_t size, MemoryType type, const std::string& name)
{
    if (size == 0) {
        return B_BAD_VALUE;
    }
    
    // Check for overlap with existing regions
    for (const auto& region : fRegions) {
        if (!(start >= region.end || start + size <= region.start)) {
            printf("[ENHANCED_ADDRESS_SPACE] Region overlap detected: 0x%x-0x%x with existing 0x%x-0x%x\n",
                   start, start + size, region.start, region.end);
            return B_BAD_VALUE;
        }
    }
    
    MemoryRegion region;
    region.start = start;
    region.end = start + size;
    region.size = size;
    region.type = type;
    region.protection = PROT_READ | PROT_WRITE; // Default protection
    region.host_ptr = fMemory ? fMemory + start : nullptr;
    region.name = name;
    
    fRegions.push_back(region);
    fAddressToRegion[start] = fRegions.size() - 1;
    
    printf("[ENHANCED_ADDRESS_SPACE] Added region: 0x%x-0x%x (%zu KB) %s\n",
           start, start + size, size / 1024, name.c_str());
    
    return B_OK;
}

EnhancedDirectAddressSpace::MemoryRegion* EnhancedDirectAddressSpace::FindRegion(uintptr_t address)
{
    for (auto& region : fRegions) {
        if (address >= region.start && address < region.end) {
            return &region;
        }
    }
    return nullptr;
}

const EnhancedDirectAddressSpace::MemoryRegion* EnhancedDirectAddressSpace::FindRegion(uintptr_t address) const
{
    for (const auto& region : fRegions) {
        if (address >= region.start && address < region.end) {
            return &region;
        }
    }
    return nullptr;
}