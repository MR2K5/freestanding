project(
    'avr',
    'c',
    'cpp',
    default_options: ['cpp_std=c++26', 'c_std=c23', 'warning_level=3'],
    version: '0.1',
    meson_version: '>=1.11',
)

add_project_arguments(
    '-Wno-gcc-compat',
    '-Wno-c11-extensions',
    '-Wno-c99-extensions',
    '-Wno-user-defined-literals',
    language: 'cpp',
)

inc = include_directories('include', '.')
std_inc = inc

conf_data = configuration_data()
conf_data.set10('_STD_HAS_HEAP', get_option('has_heap'))
conf_data.set10('_STD_HAS_THREADS', get_option('has_threads'))
config_h = configure_file(output: '__autoconfig.hpp', configuration: conf_data)

if get_option('has_heap')
    tlsf = subproject('tlsf')
endif

subdir('src')
subdir('test')

if not meson.is_subproject()
    executable(
        'main',
        sources: 'src/main.cpp',
        include_directories: inc,
        dependencies: std_dep,
    )
endif
