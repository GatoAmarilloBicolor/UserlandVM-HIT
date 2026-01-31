# Architecture-specific implementations

subdir('x86_32')
subdir('arm')
subdir('riscv')

# Factory implementation
arch_factory_sources = [
    'ArchitectureFactory.cpp',
]

arch_lib = static_library(
    'uvm_arch',
    arch_factory_sources,
    link_with: [
        x86_32_lib,
        arm_lib,
        riscv_lib,
    ],
    install: false
)
