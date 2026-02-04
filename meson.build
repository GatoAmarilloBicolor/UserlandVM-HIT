project(
	'UserlandVM', 'cpp', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++2a']
 )

cpp = meson.get_compiler('cpp')

# RISC-V disabled for now - focus on x86-32 first
# subdir('rvvm')
# Temu disabled for now - focus on x86-32 first
# subdir('temu')
# addon_rvvm = shared_library('temu',
# 	'VirtualCpuRiscVTemu.cpp',	
# 	cpp_args: temu_defines + ['-DMAX_XLEN=64'],
# 	link_with: [temu],
# 	gnu_symbol_visibility: 'hidden',
# 	install: true
# )


addon_temu = shared_library('temu',
	'VirtualCpuRiscVTemu.cpp',	
	cpp_args: temu_defines + ['-DMAX_XLEN=64'],
	link_with: [temu],
	gnu_symbol_visibility: 'hidden',
	install: true
)
dep_addon_temu = declare_dependency(
	link_with: addon_temu
)

# RISC-V components disabled for now - focus on x86-32
# addon_rvvm = shared_library('rvvm',
# 	'VirtualCpuRiscVRvvm.cpp',
# 	cpp_args: rvvm_defines + ['-DMAX_XLEN=64'],
# 	link_with: [rvvm],
# 	gnu_symbol_visibility: 'hidden',
# 	install: true
# )

# addon_rvvm2 = shared_library('rvvm',
# 	'VirtualCpuRiscVRvvm2.cpp',
# 	'Referenceable.cpp',
# 	dependencies: [dep_rvvm],
# 	gnu_symbol_visibility: 'hidden',
# 	install: true
# )
# dep_addon_rvvm = declare_dependency(
# 	link_with: addon_rvvm
# )
# dep_addon_rvvm2 = declare_dependency(
# 	link_with: addon_rvvm2
# )


executable(
	'UserlandVM',
	'Loader.cpp',
	'Syscalls.cpp',
	'DynamicLinker.cpp',
	'RelocationProcessor.cpp',
	'DirectAddressSpace.cpp',
	'CommpageManager.cpp',
	'TLSSetup.cpp',
	'ExecutionBootstrap.cpp',
	'VirtualCpuX86Native.cpp',
	'VirtualCpuX86NativeCode.S',
	'VirtualCpuX86Test.cpp',
	'X86_32GuestContext.cpp',
	'Haiku32SyscallDispatcher.cpp',
	'InterpreterX86_32.cpp',
	'StubFunctions.cpp',
	'DebugOutput.cpp',
	'FPUInstructionHandler.cpp',
	'FloatingPointUnit.cpp',
	'OptimizedX86Executor.cpp',
	'HaikuGUISyscalls.cpp',
	'HaikuGUIBackend.cpp',
	'Main.cpp',
	include_directories: [
		'external/haiku/headers',
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)
