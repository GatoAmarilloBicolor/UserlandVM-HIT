/*
 * DynamicLibraryResolver.h
 * 
 * Carga librerías 32-bit desde sysroot para la aplicación guest
 * Integrado en el interpreter - cuando guest llama a función,
 * resuelve desde las librerías del sysroot
 */

#pragma once

#include <string>
#include <map>
#include <vector>

class DynamicLibraryResolver {
public:
    DynamicLibraryResolver(const std::string& sysroot_path);
    ~DynamicLibraryResolver();
    
    // Cargar una librería dinámica del sysroot
    bool LoadLibrary(const std::string& lib_name);
    
    // Resolver símbolo en una librería
    // Busca en las librerías cargadas del sysroot
    uint32_t ResolveSymbol(const char* symbol_name, const char* lib_name);
    
    // Resolver símbolo Be API específicamente
    uint32_t ResolveBeAPISymbol(const char* symbol_name);
    
    // Get memoria de la librería en guest context
    uint32_t GetLibraryBaseAddress(const char* lib_name);
    
private:
    std::string sysroot;
    
    // Librerías cargadas: nombre -> info
    struct LibraryInfo {
        std::string name;
        std::string path;
        std::vector<uint8_t> data;  // Contenido del .so
        uint32_t guest_base;         // Dónde cargó en guest memory
        std::map<std::string, uint32_t> symbols; // Symbol -> offset
    };
    
    std::map<std::string, LibraryInfo> loaded_libs;
    
    // Cargar tabla de símbolos de un .so 32-bit
    bool ParseELFSymbols(const std::string& lib_path, LibraryInfo& info);
    
    // Encontrar librería en sysroot
    std::string FindLibraryPath(const std::string& lib_name);
};
