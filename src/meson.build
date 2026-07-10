std_src = files(
    '__cxa_atexit.cpp',
    'cassert.cpp',
    'cstdlib.cpp',
    'cstring.cpp',
    'exception.cpp',
    'exit.cpp',
    'new.cpp',
    'rtti.cpp',
)

conf_data = configuration_data()
conf_data.set10('_STD_ENABLE_HEAP', get_option('enable_heap'))
conf_data.set10('_STD_HAS_ATOMICS', get_option('has_atomics'))
conf_data.set10('_STD_HAS_EXCEPTIONS', get_option('has_exceptions'))
config_h = configure_file(output: 'config.hpp', configuration: conf_data)

if host_machine.cpu_family() == 'avr'
    subdir('avr')
elif host_machine.cpu_family() == 'x86_64'
    subdir('x64')
endif

std_deps = [abi_dep]
if get_option('enable_heap')
    std_deps += tlsf_dep
endif

std = library(
    'std',
    sources: [std_src, config_h],
    include_directories: std_inc,
    dependencies: std_deps,
)

std_dep = declare_dependency(
    link_with: std,
    include_directories: [include_directories('.'), std_inc],
    dependencies: std_deps,
    sources: config_h,
)
