if meson.is_cross_build()
    subdir_done()
endif

test_rtti = executable(
    'test-rtti',
    sources: files('test_rtti.cpp'),
    dependencies: [std_dep],
    cpp_args: '-frtti',
)

test('rtti', test_rtti)
