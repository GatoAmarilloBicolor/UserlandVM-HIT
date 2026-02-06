/*
 * UserlandVM-HIT - Haiku Userland Virtual Machine
 * Versión básica de 32 bits para ejecutar programas Haiku
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <vector>
#include <cstdint>

// Estructuras ELF básicas
struct ELFHeader {
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

// Memoria guest simulada
class GuestMemory {
private:
    std::vector<uint8_t> memory;
    static const uint32_t MEMORY_SIZE = 0x80000000; // 2GB

public:
    GuestMemory() : memory(MEMORY_SIZE, 0) {}
    
    bool Write(uint32_t addr, const void* data, size_t size) {
        if (addr + size > MEMORY_SIZE) return false;
        std::memcpy(&memory[addr], data, size);
        return true;
    }
    
    bool Read(uint32_t addr, void* data, size_t size) {
        if (addr + size > MEMORY_SIZE) return false;
        std::memcpy(data, &memory[addr], size);
        return true;
    }
    
    uint8_t* GetPointer(uint32_t addr) {
        if (addr >= MEMORY_SIZE) return nullptr;
        return &memory[addr];
    }
    
    void Write32(uint32_t addr, uint32_t value) {
        if (addr + 4 <= MEMORY_SIZE) {
            *reinterpret_cast<uint32_t*>(&memory[addr]) = value;
        }
    }
    
    uint32_t Read32(uint32_t addr) {
        if (addr + 4 <= MEMORY_SIZE) {
            return *reinterpret_cast<uint32_t*>(&memory[addr]);
        }
        return 0;
    }
};

// Intérprete x86-32 básico
class X86_32Interpreter {
private:
    struct Registers {
        uint32_t eax, ebx, ecx, edx;
        uint32_t esi, edi, ebp;
        uint32_t esp;
        uint32_t eip;
        uint32_t eflags;
    } regs;
    
    GuestMemory& memory;
    
public:
    X86_32Interpreter(GuestMemory& mem) : memory(mem) {
        std::memset(&regs, 0, sizeof(regs));
        regs.esp = 0x70000000; // Stack al final de memoria
    }
    
    uint32_t GetRegister32(uint8_t reg) {
        switch (reg) {
            case 0: return regs.eax;
            case 1: return regs.ecx;
            case 2: return regs.edx;
            case 3: return regs.ebx;
            case 4: return regs.esp;
            case 5: return regs.ebp;
            case 6: return regs.esi;
            case 7: return regs.edi;
            default: return 0;
        }
    }
    
    bool LoadELF(const std::string& filename, uint32_t& entryPoint, bool& needsDynamic) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;
        
        ELFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        // Check for PT_INTERP segment
        needsDynamic = false;
        for (int i = 0; i < header.phnum; i++) {
            ProgramHeader phdr;
            file.seekg(header.phoff + i * sizeof(ProgramHeader));
            file.read(reinterpret_cast<char*>(&phdr), sizeof(ProgramHeader));
            
            if (phdr.type == 3) { // PT_INTERP = 3
                needsDynamic = true;
                printf("[ELF] Program requires dynamic linking (PT_INTERP found)\n");
                break;
            }
        }
        
        file.seekg(0);
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        // Verificar ELF
        if (header.ident[0] != 0x7F || header.ident[1] != 'E' || 
            header.ident[2] != 'L' || header.ident[3] != 'F') {
            return false;
        }
        
        // Verificar x86-32
        if (header.machine != 0x03) { // EM_386
            return false;
        }
        
        entryPoint = header.entry;
        
        // Cargar segmentos de programa
        file.seekg(header.phoff);
        for (int i = 0; i < header.phnum; i++) {
            ProgramHeader ph;
            file.read(reinterpret_cast<char*>(&ph), sizeof(ph));
            
            if (ph.type == 1) { // PT_LOAD
                // Leer datos del segmento
                std::vector<uint8_t> segmentData(ph.filesz);
                file.seekg(ph.offset);
                file.read(reinterpret_cast<char*>(segmentData.data()), ph.filesz);
                
                // Escribir en memoria guest
                memory.Write(ph.vaddr, segmentData.data(), ph.filesz);
                
                // Rellenar con ceros si memsz > filesz
                if (ph.memsz > ph.filesz) {
                    std::vector<uint8_t> zeroFill(ph.memsz - ph.filesz, 0);
                    memory.Write(ph.vaddr + ph.filesz, zeroFill.data(), zeroFill.size());
                }
                
                std::cout << "Loaded segment at 0x" << std::hex << ph.vaddr 
                         << ", size 0x" << ph.memsz << std::dec << std::endl;
            }
        }
        
        return true;
    }
    
    bool Run(uint32_t entryPoint) {
        regs.eip = entryPoint;
        std::cout << "Starting execution at 0x" << std::hex << entryPoint << std::dec << std::endl;
        
        uint32_t instructionCount = 0;
        const uint32_t MAX_INSTRUCTIONS = 1000000; // Limitar para evitar loops infinitos
        
        while (instructionCount < MAX_INSTRUCTIONS && !ShouldExit()) {
            FetchDecodeExecute();
            instructionCount++;
            
            if (instructionCount % 100000 == 0) {
                std::cout << "Executed " << instructionCount << " instructions..." << std::endl;
            }
        }
        
        std::cout << "Execution completed after " << instructionCount << " instructions" << std::endl;
        std::cout << "Exit code: 0x" << std::hex << regs.eax << std::dec << std::endl;
        
        return true;
    }
    
private:
    bool ShouldExit() {
        // Simplificado: salir si EIP es inválido
        return regs.eip == 0 || regs.eip >= 0x80000000;
    }
    
    void FetchDecodeExecute() {
        uint8_t opcode;
        if (!memory.Read(regs.eip, &opcode, 1)) {
            regs.eip = 0; // Error
            return;
        }
        regs.eip++;
        
        // Implementación básica de algunas instrucciones clave
        switch (opcode) {
            case 0xB8: case 0xB9: case 0xBA: case 0xBB: // MOV reg32, imm32
            case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
                uint8_t reg_dest = opcode - 0xB8;
                uint32_t imm_value;
                if (!memory.Read(regs.eip, &imm_value, 4)) { regs.eip = 0; return; }
                else regs.eip += 4;
                SetRegister32(reg_dest, imm_value);
                break;
            }
                

                
            case 0xC3: // RET
                if (!memory.Read(regs.esp, &regs.eip, 4)) regs.eip = 0;
                else regs.esp += 4;
                break;
                
            case 0x31: { // XOR r32, r/m32
                uint8_t modrm;
                if (!memory.Read(regs.eip, &modrm, 1)) { regs.eip = 0; return; }
                regs.eip++;
                
                uint8_t reg = (modrm >> 3) & 7;
                uint8_t rm = modrm & 7;
                
                if (reg == rm && (modrm & 0xC0) == 0xC0) { // XOR reg, reg
                    SetRegister32(reg, 0);
                } else {
                    regs.eip++; // Saltar por ahora
                }
                break;
            }
            
            case 0xCD: { // INT imm8 (syscall)
                uint8_t int_num;
                if (!memory.Read(regs.eip, &int_num, 1)) { regs.eip = 0; return; }
                regs.eip++;
                
                if (int_num == 0x80) { // Linux syscall (usaremos para Haiku syscalls)
                    HandleHaikuSyscall();
                }
                break;
            }
            
            case 0x89: { // MOV r/m32, r32
                uint8_t modrm;
                if (!memory.Read(regs.eip, &modrm, 1)) { regs.eip = 0; return; }
                regs.eip++;
                
                uint8_t reg_src = (modrm >> 3) & 7;
                // Solo handle MOV reg, reg por ahora
                if ((modrm & 0xC0) == 0xC0) {
                    uint8_t reg_dest = modrm & 7;
                    uint32_t src = GetRegister32(reg_src);
                    SetRegister32(reg_dest, src);
                } else {
                    regs.eip++; // Skip complex addressing
                }
                break;
            }
            
            case 0x8B: { // MOV r32, r/m32
                uint8_t modrm;
                if (!memory.Read(regs.eip, &modrm, 1)) { regs.eip = 0; return; }
                regs.eip++;
                
                uint8_t reg = (modrm >> 3) & 7;
                // Solo handle MOV reg, reg por ahora
                if ((modrm & 0xC0) == 0xC0) {
                    uint8_t rm = modrm & 7;
                    uint32_t src = GetRegister32(rm);
                    SetRegister32(reg, src);
                } else {
                    regs.eip++; // Skip complex addressing
                }
                break;
            }
            
            case 0x50: case 0x51: case 0x52: case 0x53: // PUSH reg32
            case 0x54: case 0x55: case 0x56: case 0x57: {
                uint8_t opcode = opcode - 0x50;
                uint32_t value = GetRegister32(opcode);
                regs.esp -= 4;
                if (!memory.Write(regs.esp, &value, 4)) { regs.eip = 0; return; }
                break;
            }
            
            case 0x58: case 0x59: case 0x5A: case 0x5B: // POP reg32
            case 0x5C: case 0x5D: case 0x5E: case 0x5F: {
                uint8_t opcode = opcode - 0x58;
                uint32_t value;
                if (!memory.Read(regs.esp, &value, 4)) { regs.eip = 0; return; }
                SetRegister32(opcode, value);
                regs.esp += 4;
                break;
            }
            
            default:
                // Instrucción no implementada - saltar
                regs.eip++;
                break;
        }
    }
    
    void SetRegister32(uint8_t reg, uint32_t value) {
        switch (reg) {
            case 0: regs.eax = value; break;
            case 3: regs.ebx = value; break;
            case 1: regs.ecx = value; break;
            case 2: regs.edx = value; break;
            case 4: regs.esp = value; break;
            case 5: regs.ebp = value; break;
            case 6: regs.esi = value; break;
            case 7: regs.edi = value; break;
        }
    }
    
    void HandleHaikuSyscall() {
        uint32_t syscall_num = regs.eax;
        
        printf("[SYSCALL] syscall %d (ebx=0x%x, ecx=0x%x, edx=0x%x)\n", 
               syscall_num, regs.ebx, regs.ecx, regs.edx);
        
        switch (syscall_num) {
            case 1: // Haiku exit syscall
                printf("[SYSCALL] exit(%d)\n", regs.ebx);
                regs.eip = 0; // Stop execution
                break;
                
            case 4: // Haiku write syscall  
                {
                    uint32_t fd = regs.ebx;
                    uint32_t buf = regs.ecx;
                    uint32_t count = regs.edx;
                    
                    printf("[SYSCALL] write(fd=%d, buf=0x%x, count=%d)\n", fd, buf, count);
                    
                    // Simple implementation - write to stdout
                    if (fd == 1 || fd == 2) {
                        char* data = new char[count + 1];
                        if (memory.Read(buf, data, count)) {
                            data[count] = '\0';
                            printf("%s", data);
                        }
                        delete[] data;
                        regs.eax = count; // bytes written
                    } else {
                        regs.eax = -1; // error
                    }
                    break;
                }
                
            default:
                printf("[SYSCALL] unsupported syscall %d\n", syscall_num);
                regs.eax = -1; // ENOSYS
                break;
        }
    }
    

};

void printUsage(const char* program) {
    std::cout << "UserlandVM-HIT - Haiku Userland Virtual Machine (32-bit)" << std::endl;
    std::cout << "Uso: " << program << " <programa_haiku>" << std::endl;
    std::cout << std::endl;
    std::cout << "Soporta programas Haiku x86-32 (estáticos y dinámicos)" << std::endl;
}

    int main(int argc, char* argv[]) {
        if (argc != 2) {
            printUsage(argv[0]);
            return 1;
        }
        
        std::cout << "=== UserlandVM-HIT (32-bit) ===" << std::endl;
        std::cout << "Cargando programa Haiku: " << argv[1] << std::endl;
        
        GuestMemory memory;
        X86_32Interpreter interpreter(memory);
        
        uint32_t entryPoint;
        bool needsDynamic;
        if (!interpreter.LoadELF(argv[1], entryPoint, needsDynamic)) {
            std::cerr << "Error: No se pudo cargar el programa ELF" << std::endl;
            return 1;
        }
        
        std::cout << "Punto de entrada: 0x" << std::hex << entryPoint << std::dec << std::endl;
        std::cout << "Dynamic linking requerido: " << (needsDynamic ? "SÍ" : "NO") << std::endl;
        std::cout << "Iniciando ejecución..." << std::endl;
        
        if (needsDynamic) {
            std::cout << "⚠️  ESTE PROGRAMA NECESITA ENLACE DINÁMICO" << std::endl;
            std::cout << "       UserlandVM-HIT solo tiene soporte básico PT_INTERP" << std::endl;
            std::cout << "       Requiere implementación completa para ejecutar" << std::endl;
        }
        
        if (!interpreter.Run(entryPoint)) {
            std::cerr << "Error: Falló la ejecución" << std::endl;
            return 1;
        }
        
        std::cout << "Ejecución completada" << std::endl;
        return 0;
    }
    
    std::cout << "=== UserlandVM-HIT (32-bit) ===" << std::endl;
    std::cout << "Cargando programa Haiku: " << argv[1] << std::endl;
    
    GuestMemory memory;
    X86_32Interpreter interpreter(memory);
    
    uint32_t entryPoint;
    if (!interpreter.LoadELF(argv[1], entryPoint, needsDynamic)) {
        std::cerr << "Error: No se pudo cargar el programa ELF" << std::endl;
        return 1;
    }
    
    std::cout << "Punto de entrada: 0x" << std::hex << entryPoint << std::dec << std::endl;
    std::cout << "Dynamic linking requerido: " << (needsDynamic ? "SÍ" : "NO") << std::endl;
    std::cout << "Iniciando ejecución..." << std::endl;
    
    if (!interpreter.Run(entryPoint)) {
        std::cerr << "Error: Falló la ejecución" << std::endl;
        return 1;
    }
    
    std::cout << "Ejecución completada" << std::endl;
    return 0;
}