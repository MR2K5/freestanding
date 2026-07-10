project(
    'avr',
    'c',
    'cpp',
    default_options: ['cpp_std=c++26', 'c_std=c23'],
    version: '0.1',
    meson_version: '>=1.11',
)

inc = include_directories('include')
std_inc = inc

if get_option('enable_heap')
    subdir('third-party/tlsf')
endif
subdir('src')
subdir('test')

executable(
    'main',
    sources: 'src/main.cpp',
    include_directories: inc,
    dependencies: std_dep,
)
