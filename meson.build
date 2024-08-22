project('rhi_sc', 'cpp', default_options: ['cpp_std=c++20'])
rhi_dep = dependency('pistachio_rhi')
deps = [rhi_dep]
if build_machine.system() != 'windows'
    deps += dependency('shaderc')
endif

includes = [
    'include/'
]
lib_src = [

]
exe_src = [
    'src/app.cpp'
]
if build_machine.system() != 'windows'
    lib_src += 'src/backend/shaderc/rhi_sc.cpp'
endif
librhi_sc = library('rhi_scpp', lib_src, include_directories: include_directories(includes), dependencies: deps)

rhi_sc_dep = declare_dependency(include_directories: include_directories(includes), link_with: librhi_sc, dependencies: deps)
exe_deps = [rhi_sc_dep, dependency('argparse')]
rhi_sc_exe = executable('rhi_sc', exe_src, dependencies: exe_deps)