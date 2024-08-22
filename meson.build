project('rhi_sc', 'cpp')
rhi_dep = dependency('pistachio_rhi')
deps = [rhi_dep]
if build_machine.system() != 'windows'
    deps += dependency('shaderc')
endif

includes = [
    'include/'
]
lib_src = []
if build_machine.system() != 'windows'
    lib_src += 'src/backend/shaderc/rhi_sc.cpp'
endif
executable('rhi_sc')
library('librhi_sc', lib_src, include_directories: include_directories(includes), dependencies: deps)

rhi_sc_dep = declare_dependency(include_directories: include_directories(includes))