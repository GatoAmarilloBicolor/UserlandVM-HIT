# UserlandVM-HIT - Userland Emulator for Haiku OS

A powerful userland emulator that allows running Haiku x86-32 and RISC-V applications on Linux systems through binary translation and system call emulation.

## 🚀 Features

- **Multi-architecture support**: x86-32 (with JIT), RISC-V (via RVVM), ARM (planned)
- **Complete syscall emulation**: 286 Haiku syscalls including GUI, networking, and IPC
- **Modular GUI backend**: SDL2, stub, and extensible backend system
- **Dynamic linking**: Full ELF loader with symbol resolution
- **FPU support**: Hardware-accelerated floating point emulation
- **TLS support**: Thread-local storage handling
- **High performance**: Optimized execution paths and JIT compilation

## 📋 Table of Contents

- [Quick Start](#-quick-start)
- [Installation](#-installation)
- [Building](#-building)
- [Sysroot Setup](#-sysroot-setup)
- [Usage](#-usage)
- [Testing](#-testing)
- [Optimization](#-optimization)
- [Architecture](#-architecture)
- [Contributing](#-contributing)

## 🎯 Quick Start

```bash
# Clone the repository
git clone https://github.com/GatoAmarilloBicolor/UserlandVM-HIT.git
cd UserlandVM-HIT

# Install dependencies (Ubuntu/Debian)
sudo apt install gcc g++ meson ninja-build libsdl2-dev libpixman-1-dev libpango1.0-dev libfreetype6-dev nasm

# Build the emulator
meson setup builddir
ninja -C builddir

# Setup minimal sysroot (80MB optimized package)
./create_minimal_sysroot.sh

# Verify the sysroot
./verify_sysroot.sh --full

# Run tests
./test_userlandvm.sh

# Test with a Haiku application
./builddir/UserlandVM sysroot/haiku32/apps/Terminal
```

## 📦 Installation

### Prerequisites

**Required Dependencies:**
- GCC or Clang compiler
- Meson build system and Ninja
- SDL2 development libraries
- Basic development tools (make, pkg-config)

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install gcc g++ meson ninja-build libsdl2-dev libpixman-1-dev libpango1.0-dev libfreetype6-dev nasm
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc gcc-c++ meson ninja libsdl2-devel pixman-devel pango-devel freetype-devel nasm
```

**Arch Linux:**
```bash
sudo pacman -S gcc meson ninja sdl2 pixman pango freetype nasm
```

### Clone and Build

```bash
git clone https://github.com/GatoAmarilloBicolor/UserlandVM-HIT.git
cd UserlandVM-HIT
meson setup builddir
ninja -C builddir
```

## 🔨 Building

### Standard Build

```bash
# Configure build
meson setup builddir

# Compile
ninja -C builddir

# Install (optional)
sudo ninja -C builddir install
```

### Build Options

```bash
# Debug build
meson setup --buildtype=debug builddir

# Release build with optimizations
meson setup --buildtype=release builddir

# Custom installation prefix
meson setup --prefix=/usr/local builddir
```

### Cross-compilation

The project supports multiple target architectures:

```bash
# Build specific architecture (default is host architecture)
meson setup builddir -Darch=x86_32    # x86-32 target
meson setup builddir -Darch=riscv64    # RISC-V 64-bit target
meson setup builddir -Darch=arm64       # ARM 64-bit target (experimental)
```

## 🏗️ Sysroot Setup

### Option 1: Full Sysroot (1.2GB)

```bash
# Download complete Haiku sysroot
./download_haiku_sysroot.sh

# Extract and setup
./setup_sysroot.sh
```

### Option 2: Optimized Sysroot (60-80MB) - **RECOMMENDED**

```bash
# Create minimal but functional sysroot
./create_minimal_sysroot.sh

# This creates:
# - userlandvm-sysroot-minimal.tar.xz (~60MB) - Essential components only
# - userlandvm-sysroot-full.tar.xz (~80MB)    - Full GUI support
```

#### Configuration Options

```bash
# Minimal mode (60MB) - CLI and basic GUI
MINIMAL_MODE=true ./create_minimal_sysroot.sh

# Full mode (80MB) - Complete Haiku environment
MINIMAL_MODE=false ./create_minimal_sysroot.sh

# Custom languages
INCLUDE_LANGUAGES="en es fr de it pt" ./create_minimal_sysroot.sh

# Compression type
COMPRESSION=xz ./create_minimal_sysroot.sh    # Best compression
COMPRESSION=gz ./create_minimal_sysroot.sh    # Fast compression
```

### Option 3: Extract from Package

```bash
# Extract downloaded optimized package
tar -xf userlandvm-sysroot-minimal.tar.xz -C .

# Or extract full package
tar -xf userlandvm-sysroot-full.tar.xz -C .
```

## 🎮 Usage

### Basic Usage

```bash
# Run with default sysroot
./builddir/UserlandVM [path_to_haiku_binary]

# Run with custom sysroot
./builddir/UserlandVM --sysroot=/path/to/sysroot [binary]

# Run in debug mode
./builddir/UserlandVM --debug --sysroot=sysroot/haiku32 [binary]

# Show help
./builddir/UserlandVM --help
```

### Advanced Options

```bash
# Specify execution engine
./builddir/UserlandVM --engine=interpreter [binary]      # Basic interpreter
./builddir/UserlandVM --engine=jit [binary]           # JIT compilation
./builddir/UserlandVM --engine=rvvm [binary]           # RISC-V emulation

# GUI backend selection
./builddir/UserlandVM --gui=sdl2 [binary]            # SDL2 backend (default)
./builddir/UserlandVM --gui=stub [binary]             # Stub backend (headless)

# Performance settings
./builddir/UserlandVM --jit-threshold=1000 [binary]      # JIT compilation threshold
./builddir/UserlandVM --timeout=60 [binary]              # Execution timeout

# Memory settings
./builddir/UserlandVM --memory=256M [binary]           # Memory limit
./builddir/UserlandVM --stack-size=8M [binary]         # Stack size
```

### Examples

```bash
# Run Haiku Terminal
./builddir/UserlandVM sysroot/haiku32/apps/Terminal

# Run custom Haiku application
./builddir/UserlandVM --sysroot=optimized_sysroot/haiku32 my_haiku_app

# Debug application startup
./builddir/UserlandVM --debug --timeout=10 ./test_app

# Performance benchmark
time ./builddir/UserlandVM --engine=jit my_haiku_app
```

## 🧪 Testing

### Comprehensive Test Suite

```bash
# Run full test suite
./test_userlandvm.sh

# Run with specific options
./test_userlandvm.sh --verbose --timeout=60

# Skip GUI tests (for headless systems)
./test_userlandvm.sh --skip-gui

# Custom sysroot
./test_userlandvm.sh --sysroot=/path/to/sysroot

# Quick tests only
./test_userlandvm.sh --quick
```

### Test Categories

1. **Basic Functionality**: Memory management, file operations, math functions
2. **System Calls**: Process management, IPC, networking syscalls
3. **Haiku Applications**: Terminal, DeskCalc, StyledEdit
4. **Performance**: Startup time, execution speed
5. **Architecture Support**: x86-32, RISC-V, ARM compatibility
6. **GUI Functionality**: Window management, drawing, input handling

### Test Results

The test suite generates detailed reports with:
- Pass/fail statistics
- Performance metrics
- Compatibility scores
- Detailed error information

## 🔧 Optimization

### Sysroot Optimization

The optimized sysroot generator reduces size while maintaining functionality:

| Component | Original | Optimized | Reduction |
|-----------|----------|------------|------------|
| Total Size | 1.2GB | 60-80MB | 93-95% |
| Libraries | 141MB | 15MB | 89% |
| Fonts | 192MB | 5MB | 97% |
| Locales | 110MB | 4MB | 96% |

### Runtime Optimization

```bash
# Enable JIT for maximum performance
./builddir/UserlandVM --engine=jit --jit-threshold=100 [binary]

# Optimize memory usage
./builddir/UserlandVM --memory=128M --stack-size=4M [binary]

# Disable debugging in production
export NODEBUG=1
./builddir/UserlandVM [binary]
```

### Development Optimization

```bash
# Use optimized build
meson setup --buildtype=release -Db_lto=true builddir

# Enable link-time optimization
ninja -C builddir

# Profile for bottlenecks
./builddir/UserlandVM --profile [binary]
```

## 🏗️ Architecture

### Core Components

```
┌─────────────────────────────────────────────────┐
│             UserlandVM-HIT              │
├─────────────────────────────────────────────┤
│  Application Layer                      │
│  ┌─────────────┐  ┌─────────────┐ │
│  │   Loader    │  │   GUI       │ │
│  └─────────────┘  └─────────────┘ │
├─────────────────────────────────────────────┤
│  Execution Layer                       │
│  ┌─────────────┐  ┌─────────────┐ │
│  │ Interpreter │  │   JIT       │ │
│  └─────────────┘  └─────────────┘ │
├─────────────────────────────────────────────┤
│  System Layer                         │
│  ┌─────────────┐  ┌─────────────┐ │
│  │ Syscalls   │  │  Memory     │ │
│  └─────────────┘  └─────────────┘ │
└─────────────────────────────────────────────┘
```

### Supported Architectures

| Architecture | Status | Implementation | Performance |
|-------------|--------|----------------|--------------|
| x86-32 | ✅ Full | Native + JIT | 🟢 Excellent |
| RISC-V 64 | ✅ Full | RVVM Integration | 🟢 Excellent |
| ARM 64 | 🟡 Planned | - | - |
| x86-64 | 🟡 Planned | - | - |

### GUI Backend System

```cpp
class HaikuGUIBackend {
    virtual status_t CreateWindow(...) = 0;
    virtual status_t DrawString(...) = 0;
    virtual status_t FillRect(...) = 0;
    // ... other GUI operations
};

// Implementations:
SDL2GUIBackend     // Hardware accelerated, cross-platform
StubGUIBackend       // Headless, for testing
CosmoeGUIBackend    // Native BeOS widgets (planned)
```

## 🤝 Contributing

### Development Setup

```bash
# Clone repository
git clone https://github.com/GatoAmarilloBicolor/UserlandVM-HIT.git
cd UserlandVM-HIT

# Install development dependencies
sudo apt install gcc g++ meson ninja libsdl2-dev nasm \
                 clang-format cppcheck valgrind

# Setup pre-commit hooks
meson setup builddir
ninja -C builddir
```

### Code Style

- Use 4-space indentation
- Follow C++17 standard
- Include unit tests for new features
- Document public APIs

### Testing Changes

```bash
# Run full test suite
./test_userlandvm.sh --verbose

# Verify sysroot integrity
./verify_sysroot.sh --full

# Performance regression test
./test_userlandvm.sh --benchmark
```

### Submitting Changes

1. Fork the repository
2. Create feature branch
3. Implement changes with tests
4. Run full test suite
5. Submit pull request with description

## 📚 Documentation

- [API Reference](docs/API.md) - Complete API documentation
- [Architecture Guide](docs/ARCHITECTURE.md) - Detailed architecture description
- [Performance Guide](docs/PERFORMANCE.md) - Optimization techniques
- [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues and solutions

## 🐛 Troubleshooting

### Common Issues

**Build failures:**
```bash
# Check dependencies
meson setup builddir

# Clean and rebuild
rm -rf builddir
meson setup builddir
ninja -C builddir
```

**Runtime errors:**
```bash
# Check sysroot integrity
./verify_sysroot.sh --quick

# Enable debug mode
./builddir/UserlandVM --debug [binary]
```

**Performance issues:**
```bash
# Use JIT engine
./builddir/UserlandVM --engine=jit [binary]

# Optimize sysroot
./create_minimal_sysroot.sh MINIMAL_MODE=true
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Haiku OS project for syscall specifications
- RVVM project for RISC-V emulation support
- SDL2 for cross-platform graphics
- The open-source community for contributions and feedback

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/GatoAmarilloBicolor/UserlandVM-HIT/issues)
- **Discussions**: [GitHub Discussions](https://github.com/GatoAmarilloBicolor/UserlandVM-HIT/discussions)
- **Wiki**: [Project Wiki](https://github.com/GatoAmarilloBicolor/UserlandVM-HIT/wiki)

---

**UserlandVM-HIT** - Bringing Haiku applications to every platform. 🚀