project('CM4all Commence', 'cpp', version: '0.2',
  meson_version: '>= 1.2',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',

    # This is only here to build subprojects as static libraries
    'default_library=static',

    'fmt:cpp_std=c++23',
  ],
)

compiler = meson.get_compiler('cpp')

debug = get_option('b_ndebug') != 'true'

common_flags = [
  '-D_GNU_SOURCE',
]

test_common_flags = [
  '-Wcast-align',
  '-Wcast-qual',
  '-Wdouble-promotion',
  '-Wfloat-equal',
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wmissing-format-attribute',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wundef',
  '-Wunused',
  '-Wvla',
  '-Wwrite-strings',

  '-Wno-unused-result',

  # clang specific warning options:
  '-Wunreachable-code-aggressive',
  '-Wused-but-marked-unused',
]

test_global_common_flags = [
  '-fno-omit-frame-pointer',
  '-fvisibility=hidden',
]

add_global_link_arguments(
  compiler.get_supported_link_arguments(
    # make relocations read-only (hardening)
    '-Wl,-z,relro',

    # no lazy binding, please - not worth it for a daemon
    '-Wl,-z,now',
  ),
  language: 'cpp'
)

if not debug
  test_global_common_flags += [
    '-ffunction-sections',
    '-fdata-sections',
  ]

  add_global_link_arguments(
    compiler.get_supported_link_arguments(
      '-Wl,--gc-sections',
      '-Wl,--icf=all',
    ),
    language: 'cpp'
  )
endif

test_global_cxxflags = test_global_common_flags + [
]

test_cxxflags = test_common_flags + [
  '-fno-threadsafe-statics',
  '-fmerge-all-constants',

  '-Wcomma-subscript',
  '-Wextra-semi',
  '-Wmismatched-tags',
  '-Woverloaded-virtual',
  '-Wsign-promo',
  '-Wvolatile',
  '-Wvirtual-inheritance',

  '-Wno-missing-field-initializers',

  # a vtable without a dtor is just fine
  '-Wno-non-virtual-dtor',

  # the only warnings we got from this are from formatted error
  # messages, and their truncation is harmless
  '-Wno-format-truncation',

  # clang specific warning options:
  '-Wcomma',
  '-Wheader-hygiene',
  '-Winconsistent-missing-destructor-override',
]

add_global_arguments(common_flags, language: 'cpp')
add_global_arguments(compiler.get_supported_arguments(test_global_cxxflags), language: 'cpp')
add_project_arguments(compiler.get_supported_arguments(test_cxxflags), language: 'cpp')

inc = include_directories('.', 'src', 'libcommon/src')

libcommon_enable_json = get_option('json')
libcommon_require_mariadb = get_option('mariadb')

libcrypt = dependency('libcrypt', required: get_option('libcrypt'))
libsodium = dependency('libsodium', required: get_option('sodium'))

subdir('libcommon/src/util')
subdir('libcommon/src/lib/fmt')
subdir('libcommon/src/lib/nlohmann_json')
subdir('libcommon/src/lua')
subdir('libcommon/src/lib/mariadb')

if nlohmann_json_dep.found()
  subdir('libcommon/src/lua/json')
else
  lua_json_dep = nlohmann_json_dep
endif

if mariadb_dep.found()
  subdir('libcommon/src/lua/mariadb')
else
  lua_mariadb_dep = mariadb_dep
endif

subdir('libcommon/src/io')
subdir('libcommon/src/system')
subdir('libcommon/src/net')

conf = configuration_data()
conf.set('HAVE_JSON', nlohmann_json_dep.found())
conf.set('HAVE_LIBCRYPT', libcrypt.found())
conf.set('HAVE_MARIADB', mariadb_dep.found())
conf.set('HAVE_SODIUM', libsodium.found())
configure_file(output: 'config.h', configuration: conf)

sources = []

if libsodium.found()
  sources += 'src/PwHash.cxx'
endif

executable('cm4all-commence',
  'src/CommandLine.cxx',
  'src/Main.cxx',
  'src/Library.cxx',
  'src/Path.cxx',
  'src/Template.cxx',
  'src/Random.cxx',
  sources,
  include_directories: inc,
  dependencies: [
    liblua,
    io_dep,
    util_dep,
    system_dep,
    lua_dep,
    lua_json_dep,
    lua_mariadb_dep,
    nlohmann_json_dep,
    fmt_dep,
    libsodium,
    libcrypt,
  ],
  install: true,
  install_dir: 'bin',
)
