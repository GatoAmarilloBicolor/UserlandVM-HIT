project(
	'UserlandVM', 'cpp', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++2a']
 )

cpp = meson.get_compiler('cpp')

subdir('rvvm')
subdir('temu')

dep_rvvm = declare_dependency(
	link_with: rvvm
)


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

addon_rvvm = shared_library('rvvm',
	'VirtualCpuRiscVRvvm.cpp',
	link_with: [rvvm],
	cpp_args: rvvm_defines + ['-DMAX_XLEN=64'],
	gnu_symbol_visibility: 'hidden',
	install: true
)
dep_addon_rvvm = declare_dependency(
	link_with: addon_rvvm
)

# addon_rvvm2 disabled - API mismatch with current RVVM version
# addon_rvvm2 = shared_library('rvvm2',
# 	'VirtualCpuRiscVRvvm2.cpp',
# 	'Referenceable.cpp',
# 	dependencies: [dep_rvvm],
# 	gnu_symbol_visibility: 'hidden',
# 	install: true
# )
# dep_addon_rvvm2 = declare_dependency(
# 	link_with: addon_rvvm2
# )


executable(
	'UserlandVM',
	'Loader.cpp',
	'Syscalls.cpp',
	'DynamicLinker.cpp',
	'ExecutionBootstrap.cpp',
	'VirtualCpuX86Native.cpp',
	'VirtualCpuX86NativeCode.S',
	'VirtualCpuX86Test.cpp',
	'Main.cpp',
	include_directories: [
		'external/haiku/headers/os',
		'external/haiku/headers/os/kernel',
		'external/haiku/headers/os/support',
		'external/haiku/headers/os/app',
		'external/haiku/headers/os/interface',
		'external/haiku/headers/os/storage',
		'external/haiku/headers/posix',
		'external/haiku/headers/private/system',
		'external/haiku/headers/private/system/arch/x86',
		'external/haiku/headers/private/shared',
		'external/haiku/headers/private/app',
		'external/haiku/headers/private/interface',
		'external/haiku/headers/private/kernel',
		'external/haiku/headers/private/libroot',
		'external/haiku/headers/private/runtime_loader',
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)
