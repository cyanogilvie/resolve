project('resolve', 'c',
  version:          '0.13',
  default_options:  ['c_std=gnu2y,gnu23,gnu2x', 'warning_level=3', 'werror=true'],
  meson_version:    '>=1.3.0',
)

# --- Accumulator variables for teabase ---
deps          = []
inc           = [include_directories('.')]
pkg_sources   = files('resolve.c')
teaconf_files = files('library/resolve.tcl')

cc = meson.get_compiler('c')
fs = import('fs')

if get_option('debug')
  add_project_arguments(cc.get_supported_arguments(['-ggdb3']), language: 'c')
endif

# Suppress warnings that TEA-style code commonly triggers
add_project_arguments(cc.get_supported_arguments([
  '-Wno-missing-field-initializers',
  '-Wno-deprecated-octal-literals',
]), language: 'c')

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

# _GNU_SOURCE or _BSD_SOURCE is required (even on musl) to get NI_MAXHOST/NI_MAXSERV from netdb.h
add_project_arguments('-D_GNU_SOURCE', language: 'c')

# Feature detection
anl_dep = cc.find_library('anl', required: false)
have_getaddrinfo_a = is_glibc and anl_dep.found() and cc.has_function('getaddrinfo_a', prefix: '#define _GNU_SOURCE\n#include <netdb.h>', dependencies: [anl_dep])
if have_getaddrinfo_a
  deps += anl_dep
endif

conf.set10('HAVE_GETADDRINFO_A', have_getaddrinfo_a)

conf.set10('HAVE_PIPE2', cc.has_function('pipe2', args: '-D_GNU_SOURCE', prefix: '#include <fcntl.h>\n#include <unistd.h>'))

# Type checks (needed by INT2PTR/PTR2INT macros in resolveInt.h)
conf.set('HAVE_INTPTR_T',  cc.has_type('intptr_t',  prefix: '#include <stdint.h>') ? 1 : 0)
conf.set('HAVE_UINTPTR_T', cc.has_type('uintptr_t', prefix: '#include <stdint.h>') ? 1 : 0)

tea_want_defer = true

subdir('teabase')

subdir('doc')
