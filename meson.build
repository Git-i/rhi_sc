project('rhi_sc', 'cpp', default_options: ['cpp_std=c++20'])
rhi_dep = dependency('pistachio_rhi')
deps = [rhi_dep]

includes = [
    'include/'
]
lib_src = [

]
exe_src = [
    'src/app.cpp'
]
extra_includes = []
if get_option('compiler-backend') == 'shaderc'
    lib_src += 'src/backend/shaderc/rhi_sc.cpp'
    deps += dependency('shaderc')
elif get_option('compiler-backend') == 'dxc'
    lib_src += 'src/backend/dxc/rhi_sc.cpp'
    cc = meson.get_compiler('cpp')
    dxc_lib = cc.find_library('dxcompiler', dirs: meson.current_source_dir() /'dxc_lib/')
    extra_includes += 'dxc_lib/'
    deps += dxc_lib
endif
librhi_sc = library('rhi_scpp', lib_src, include_directories: include_directories(includes + extra_includes), dependencies: deps)

rhi_sc_dep = declare_dependency(include_directories: include_directories(includes), link_with: librhi_sc, dependencies: deps)
exe_deps = [rhi_sc_dep, dependency('argparse')]
rhi_sc_exe = executable('rhi_sc', exe_src, dependencies: exe_deps)