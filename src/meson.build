std_src = files(
    '__cxa_atexit.cpp',
    'platform.cpp',
    'cassert.cpp',
    'cstdlib.cpp',
    'cstring.cpp',
    'exception.cpp',
    'new.cpp',
)

fs = import('fs')

if get_option('cpp_rtti')
    std_src += files('rtti.cpp')
endif

host_cpu = host_machine.cpu_family()

arch_overrides = files()
if fs.is_dir(host_cpu)
    subdir(host_cpu)
endif

std_deps = []
if get_option('has_heap')
    tlsf_dep = tlsf.get_variable('tlsf_dep')
    std_deps += tlsf_dep
endif

# 2. Extract base names (stems) from the preferred (arch) sources
arch_stems = []
foreach f : arch_overrides
  arch_stems += fs.stem(f)
endforeach

# 3. Build final list: start with arch sources, add generic if not overridden
final_sources = arch_overrides

foreach f : std_src
  if not arch_stems.contains(fs.stem(f))
    final_sources += f
  endif
endforeach

std = library(
    'std',
    sources: [final_sources, config_h],
    include_directories: std_inc,
    dependencies: std_deps,
)

std_dep = declare_dependency(
    link_with: std,
    include_directories: [std_inc],
    dependencies: std_deps,
    sources: config_h,
)
