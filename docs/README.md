# UserlandVM - Virtual Machine Emulator for Haiku OS

UserlandVM is a virtual machine emulator that enables running x86-32 bit binaries on Haiku OS (64-bit). The project provides an integrated execution environment for cross-architecture compatibility.

## Current Status

**Architecture:** Integrated Execution Model (Direct x86-32 emulation)  
**Target:** x86-32 binaries on Haiku x86-64  
**Build System:** Meson  
**Language:** C++20  

## Integrated Execution Model (x86-32)

UserlandVM now uses an integrated execution approach where x86-32 binaries are loaded and executed directly within the main `UserlandVM` executable. This eliminates the need for separate loader components and provides a unified execution environment.

### Key Components

- **Main Executable:** `UserlandVM` handles all execution
- **x86-32 Support:** Direct emulation through `VirtualCpuX86Native`
- **Memory Management:** Integrated address space management
- **Syscall Handling:** Direct syscall dispatching to Haiku host

## Building

### Prerequisites

- Haiku OS x86-64
- Meson build system
- C++20 compatible compiler
- Development headers

### Build Commands

```bash
# Setup build directory
meson setup builddir

# Compile
meson compile -C builddir

# Install (optional)
meson install -C builddir
```

## Usage

```bash
# Run x86-32 binary
./UserlandVM <x86-32-executable>

# Example
./UserlandVM hello_world_x86
```

## Architecture

The integrated execution model provides:

1. **Direct Binary Loading:** ELF32 parsing and loading
2. **CPU Emulation:** Full x86-32 instruction set support
3. **Memory Management:** Virtual address space simulation
4. **Syscall Translation:** Haiku syscall compatibility
5. **Error Handling:** Robust error reporting and recovery

## Development Status

- ✅ Basic x86-32 execution
- ✅ ELF32 binary loading
- ✅ Fundamental syscall support
- 🔄 Extended instruction set implementation
- 🔄 Performance optimizations

## Contributing

Contributions are welcome. Please ensure:

- Code follows C++20 standards
- Tests are included for new features
- Documentation is updated
- Haiku compatibility is maintained

## Documentation

Additional documentation is available in the `docs/` directory, including:

- Development logs
- Technical specifications
- Implementation details
- Testing procedures

## License

This project is licensed under MIT/BSD-compatible terms.