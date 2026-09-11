if get_option('cpp_eh') == 'default'
    std_src += files('eh1.cpp', 'eh2.cpp')

    eh_throw = static_library(
        'eh_throw',
        'cxa_throw.cpp',
        cpp_args: ['-funwind-tables'],
        include_directories: std_inc,
    ).extract_all_objects()
    extra_objects += eh_throw

else
    std_src += files('no_exceptions.cpp')
endif

arch_overrides += files('platform.cpp')
