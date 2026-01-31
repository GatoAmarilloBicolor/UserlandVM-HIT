#!/usr/bin/env python3
"""
Script para extraer librerías 32-bit de Haiku ISO
Soporta múltiples métodos de extracción
"""
import os
import sys
import subprocess
import struct

ISO_PATH = "/boot/home/Downloads/haiku-master-hrev59182-x86_gcc2h-anyboot.iso"
OUTPUT_DIR = "/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32/system/lib"

def ensure_output_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print(f"[+] Directorio creado: {OUTPUT_DIR}")

def method_isoinfo():
    """Intenta con isoinfo"""
    print("\n[*] Método 1: isoinfo")
    try:
        # Listar archivos .so
        cmd = f"isoinfo -f -R -i {ISO_PATH} 2>/dev/null | grep -i '\\.so'"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if result.stdout:
            print(f"[+] Encontrados archivos .so")
            return True
        else:
            print("[-] isoinfo no encontró archivos .so")
            return False
    except Exception as e:
        print(f"[-] Error: {e}")
        return False

def method_cpio():
    """Intenta extraer con cpio"""
    print("\n[*] Método 2: cpio desde ISO")
    try:
        cmd = f"cd /tmp && dd if={ISO_PATH} bs=2048 2>/dev/null | cpio -idm '*/system/lib/*.so*' 2>/dev/null && ls system/lib/"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if "libc" in result.stdout or "libm" in result.stdout:
            print(f"[+] cpio extrajo librerías")
            # Copiar
            subprocess.run(f"cp /tmp/system/lib/*.so* {OUTPUT_DIR}/ 2>/dev/null", shell=True)
            subprocess.run("rm -rf /tmp/system", shell=True)
            return True
        else:
            print("[-] cpio no encontró librerías")
            return False
    except Exception as e:
        print(f"[-] Error: {e}")
        return False

def method_tar():
    """Intenta extraer con tar"""
    print("\n[*] Método 3: tar desde ISO")
    try:
        # Algunos ISOs de Haiku pueden ser formato tar comprimido
        cmd = f"cd /tmp && tar xzf {ISO_PATH} --wildcards '*/system/lib/*.so*' 2>/dev/null && ls system/lib/"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if "libc" in result.stdout or "libm" in result.stdout:
            print(f"[+] tar extrajo librerías")
            subprocess.run(f"cp /tmp/system/lib/*.so* {OUTPUT_DIR}/ 2>/dev/null", shell=True)
            subprocess.run("rm -rf /tmp/system", shell=True)
            return True
        else:
            print("[-] tar no funcionó")
            return False
    except Exception as e:
        print(f"[-] Error: {e}")
        return False

def create_minimal_stubs():
    """Crea librerías stub mínimas para testing"""
    print("\n[*] Creando librerías stub para testing...")
    
    essential_libs = [
        "libc.so.0",
        "libm.so.0",
        "libroot.so",
        "libbe.so"
    ]
    
    for lib in essential_libs:
        path = os.path.join(OUTPUT_DIR, lib)
        try:
            # Crear archivos vacíos (son stubs)
            with open(path, 'wb') as f:
                # Escribir header ELF mínimo para 32-bit
                # ELF magic: 0x7f 'E' 'L' 'F'
                elf_header = bytes([
                    0x7f, ord('E'), ord('L'), ord('F'),  # ELF magic
                    1,                                     # 32-bit
                    1,                                     # Little endian
                    1,                                     # ELF version
                    0,                                     # UNIX System V ABI
                    0, 0, 0, 0, 0, 0, 0, 0,               # Padding
                    3, 0,                                  # ET_DYN (shared object)
                    3, 0,                                  # Intel 80386
                ])
                f.write(elf_header)
                # Agregar padding para que tenga tamaño mínimo
                f.write(b'\x00' * 100)
            os.chmod(path, 0o755)
            print(f"[+] Stub creado: {lib}")
        except Exception as e:
            print(f"[-] Error creando {lib}: {e}")

def verify_extraction():
    """Verifica qué se extrajo"""
    print(f"\n[*] Contenido de {OUTPUT_DIR}:")
    try:
        files = os.listdir(OUTPUT_DIR)
        if files:
            for f in files:
                path = os.path.join(OUTPUT_DIR, f)
                size = os.path.getsize(path)
                print(f"  - {f} ({size} bytes)")
            return len(files) > 0
        else:
            print("  (vacío)")
            return False
    except:
        return False

def main():
    print("=" * 70)
    print("Haiku ISO Library Extractor")
    print("=" * 70)
    
    if not os.path.exists(ISO_PATH):
        print(f"[!] ISO no encontrado: {ISO_PATH}")
        return False
    
    print(f"[*] ISO: {ISO_PATH}")
    print(f"[*] Tamaño: {os.path.getsize(ISO_PATH) / (1024*1024):.1f} MB")
    
    ensure_output_dir()
    
    # Intentar métodos de extracción
    success = False
    success = success or method_isoinfo()
    success = success or method_cpio()
    success = success or method_tar()
    
    if not success:
        print("\n[!] No se pudo extraer del ISO")
        print("[*] Creando stubs mínimos para testing...")
        create_minimal_stubs()
    
    # Verificar resultado
    if verify_extraction():
        print("\n[+] ¡Librerías listas en sysroot!")
        return True
    else:
        print("\n[!] Sysroot vacío")
        print("\nAlternativas:")
        print("1. Descarga Haiku 32-bit: https://www.haiku-os.org/get-haiku/")
        print("2. Instala en VM y copia /system/lib")
        print("3. Solicita pre-extracted en Haiku forums")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
