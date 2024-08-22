project('rhi_sc', 'cpp')
rhi_dep = dependency('pistachio_rhi')
deps = [rhi_dep]
if build_machine.system() != 'windows'
    deps += dependency('shaderc')
endif

includes = [
    'include/'
]
executable('rhi_sc')
library('librhi_sc')

rhi_sc_dep = declare_dependency(include_directories: include_directories(includes))