project('resolve', 'c',
  version:          '0.11',
  default_options:  ['c_std=c11', 'warning_level=3', 'werror=true'],
  meson_version:    '>=1.3.0',
)

cc = meson.get_compiler('c')
fs = import('fs')

if get_option('debug')
  add_project_arguments(cc.get_supported_arguments(['-ggdb3']), language: 'c')
endif

# Suppress warnings that TEA-style code commonly triggers
add_project_arguments('-Wno-missing-field-initializers', language: 'c')

# --- Configuration data (becomes config.h) ---
conf = configuration_data()

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

# Check for glibc and enable _GNU_SOURCE (needed for getaddrinfo_a)
is_glibc = cc.compiles('''
#include <features.h>
#if ! (defined __GLIBC__ || defined __GNU_LIBRARY__)
#  error "Not glibc"
#endif
''', name: 'glibc check')

if is_glibc
  add_project_arguments('-D_GNU_SOURCE', language: 'c')
endif

# Feature detection
anl_dep = cc.find_library('anl', required: false)
have_getaddrinfo_a = is_glibc and anl_dep.found() and cc.has_function('getaddrinfo_a', prefix: '#define _GNU_SOURCE\n#include <netdb.h>', dependencies: [anl_dep])
conf.set10('HAVE_GETADDRINFO_A', have_getaddrinfo_a)

conf.set10('HAVE_PIPE2', cc.has_function('pipe2', prefix: '#define _GNU_SOURCE\n#include <fcntl.h>\n#include <unistd.h>'))

# Type checks (needed by INT2PTR/PTR2INT macros in resolveInt.h)
conf.set('HAVE_INTPTR_T',  cc.has_type('intptr_t',  prefix: '#include <stdint.h>') ? 1 : 0)
conf.set('HAVE_UINTPTR_T', cc.has_type('uintptr_t', prefix: '#include <stdint.h>') ? 1 : 0)

# --- Accumulator variables for teabase ---
deps = []
inc = []

if have_getaddrinfo_a
  deps += anl_dep
endif

# Package sources
pkg_sources = files(
  'generic/resolve.c',
)

# --- Teabase init (must come first) ---
# Provides: pkg_dir, tclsh, tcl_dep, teaconf
#subdir('teabase/init')

# --- Teabase main (builds shared_module, sets up tests) ---
subdir('teabase')

# Write non-installed breadcrumb next to pkgIndex.tcl so dev builds
# can find the source tree's Tcl library files
run_command('sh', '-c', 'printf "%s" "$1" > "$2"', '--',
  meson.project_source_root(),
  meson.project_build_root() / 'teabase' / 'resolve_srcdir.txt',
  check: true,
)

# --- Install Tcl library files ---
install_data('library/resolve.tcl',
  install_dir: pkg_dir,
)

# --- Documentation ---
subdir('doc')
