
abi = library(
    'abi-avr',
    sources: files(
        'divmodhi4.S',
        'divmodqi4.S',
        'mulhi3.S',
        'mulqi3.S',
        'udivmodhi4.S',
        'udivmodqi4.S',
        'guard.cpp',
        'crt0.cpp'
    ),
    include_directories: std_inc
)
abi_dep = declare_dependency(link_with: abi)
