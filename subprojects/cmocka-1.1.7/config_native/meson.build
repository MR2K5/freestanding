entry = cc_native_dict
compiler = entry.get('compiler')
is_native = entry.get('native')
machine = entry.get('machine')

config = configuration_data()

if compiler.get_argument_syntax() == 'gcc'
  add_project_arguments(
    # I've explicitly skipped the duplicated -W versions when they also test
    # for the -Werror version
    compiler.get_supported_arguments(
      '-Wshadow',
      '-Wmissing-prototypes',
      '-Wcast-align',
      '-Werror=address',
      '-Werror=strict-prototypes',
      '-Werror=write-strings',
      '-Werror=implicit-function-declaration',
      '-Werror=pointer-arith',
      '-Werror=declaration-after-statement',
      '-Werror=return-type',
      '-Werror=uninitialized',
      '-Wimplicit-fallthrough',
      '-Werror=strict-overflow',
      '-Wstrict-overflow=2',
      '-Wno-format-zero-length',
      '-Wformat',
      '-Werror=format-security',
      '-Wno-gnu-zero-variadic-macro-arguments',
      '-D_GNU_SOURCE',
    ),
    language: 'c',
    native: is_native,
  )
  # We can't test the build type, so we can' add -D_FORTIFY_SOURCE=2 here
  if machine.system() == 'darwin'
    add_project_arguments(
      compiler.get_supported_arguments('-Wno-deprecated-declarations'),
      language: 'c',
      native: is_native,
    )
  endif
else
  add_project_arguments(
    '/D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
    '/D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1',
    '/D_CRT_NONSTDC_NO_WARNINGS=1',
    '/D_CRT_SECURE_NO_WARNINGS=1',
    language: 'c',
    native: is_native,
  )
endif

# TODO: solaris extensions

foreach h : [
  'assert.h',
  'dlfcn.h',
  'inttypes.h',
  'io.h',
  'malloc.h',
  'memory.h',
  'setjmp.h',
  'signal.h',
  'stdarg.h',
  'stddef.h',
  'stdint.h',
  'stdio.h',
  'stdlib.h',
  'string.h',
  'strings.h',
  'sys/stat.h',
  'sys/types.h',
  'time.h',
  'unistd.h',
]
  if compiler.check_header(h)
    config.set('HAVE_@0@'.format(h.underscorify().to_upper()), 1)
  endif
endforeach

if compiler.has_member('struct timespec', 'tv_sec', prefix: '#include <time.h>')
  config.set('HAVE_STRUCT_TIMESPEC', 1)
endif

foreach f : [
  'calloc',
  'clock_gettime',
  'exit',
  'free',
  'longjmp',
  'malloc',
  'memcpy',
  'memset',
  'setjmp',
  'siglongjmp',
  'signal',
  'strsignal',
  'strcmp',
]
  if compiler.has_function(f)
    config.set('HAVE_@0@'.format(f.underscorify().to_upper()), 1)
  endif
endforeach

foreach f : ['fprintf', 'printf', 'snprintf', 'vsnprintf']
  if compiler.has_header_symbol('stdio.h', f)
    config.set('HAVE_@0@'.format(f.underscorify().to_upper()), 1)
  endif
endforeach

threads_dep = []
if not config.has('HAVE_CLOCK_GETTIME')
  if compiler.has_function('clock_gettime', dependencies: dependency('threads'))
    config.set('HAVE_@0@'.format(f.underscorify().to_upper()), 1)
    threads_dep = dependency('threads')
  endif
endif

if compiler.get_argument_syntax() == 'msvc'
  config.set('HAVE_MSVC_THREAD_LOCAL_STORAGE', compiler.compiles('__declspec(thread) int tls;'))
else
  config.set10('HAVE_GCC_THREAD_LOCAL_STORAGE', compiler.compiles('__thread int tls;'))
endif

config.set('WORDS_SIZEOF_VOID_P', compiler.sizeof('void*'))
if machine.endian() == 'big'
  config.set('WORDS_BIGENDIAN', 1)
endif

cfile_native = configure_file(
  input: config_input_file,
  output: 'config.h',
  format: 'cmake',
  configuration: config,
)
