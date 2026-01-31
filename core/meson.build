# Core modules - architecture-agnostic
# These files are reused by all architecture implementations

core_sources = [
    'AddressSpace.cpp',
    'DirectAddressSpace.cpp',
    'DynamicLinker.cpp',
    'GuestElfLoader.cpp',
    'SymbolResolver.cpp',
    'SysrootManager.cpp',
    'DebugOutput.cpp',
    'Referenceable.cpp',
]

core_headers = [
    'GuestContextBase.h',
    'ExecutionEngineBase.h',
    'SyscallDispatcherBase.h',
    'AddressSpace.h',
    'DirectAddressSpace.h',
    'DynamicLinker.h',
    'GuestElfLoader.h',
    'SymbolResolver.h',
    'SysrootManager.h',
    'DebugOutput.h',
    'Referenceable.h',
]

core_lib = static_library(
    'uvm_core',
    core_sources,
    install: false
)
