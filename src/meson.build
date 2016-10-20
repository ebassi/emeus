public_headers = [
  'emeus-constraint.h',
  'emeus-constraint-layout.h',

  'emeus-version-macros.h',
  'emeus-types.h',
]

private_headers = [
  'emeus-constraint-private.h',
  'emeus-constraint-layout-private.h',
  'emeus-expression-private.h',
  'emeus-macros-private.h',
  'emeus-simplex-solver-private.h',
  'emeus-types-private.h',
  'emeus-utils-private.h',
  'emeus-variable-private.h',
  'emeus-vfl-parser-private.h',
]

sources = [
  'emeus-constraint.c',
  'emeus-constraint-layout.c',
  'emeus-expression.c',
  'emeus-simplex-solver.c',
  'emeus-types.c',
  'emeus-utils.c',
  'emeus-variable.c',
  'emeus-vfl-parser.c',
]

# Generated headers
configure_file(input: 'config.h.meson',
               output: 'config.h',
               configuration: conf)
configure_file(input: 'emeus-version.h.meson',
               output: 'emeus-version.h',
               configuration: version_conf,
               install: true,
               install_dir: 'include/emeus-1.0')

install_headers(public_headers + [ 'emeus.h' ], subdir: 'emeus-1.0')

libemeus = shared_library('emeus-@0@'.format(emeus_api_version),
  sources: sources + private_headers,
  version: '@0@.@1@.@2@'.format(emeus_major_version, emeus_minor_version, emeus_micro_version),
  install: true,
  dependencies: [ gtk_dep, mathlib_dep ],
  c_args: common_cflags + debug_cflags + gtk_version_cflags + [
    '-DEMEUS_COMPILATION=1',
    '-DG_LOG_USE_STRUCTURED=1',
    '-DG_LOG_DOMAIN="Emeus"',
  ],
  link_args: [ '-Wl,-Bsymbolic-functions' ])

# Internal dependency, for tests
emeus_inc = include_directories('.')
emeus_dep = declare_dependency(link_with: libemeus,
                               include_directories: [ emeus_inc ],
                               dependencies: [ gtk_dep, mathlib_dep ])

# PkgConfig file
configure_file(input: 'emeus.pc.in',
               output: 'emeus-@0@.pc'.format(emeus_api_version),
               configuration: conf,
               install: true,
               install_dir: 'lib/pkgconfig')

# Introspection
if get_option('enable-introspection')
  gir_extra_args = [
    '--c-include=emeus.h',
    '-DEMEUS_COMPILATION',
    '--cflags-begin',
    '-I' + meson.source_root() + '/src',
    '-I' + meson.build_root() + '/src',
    '--cflags-end',
  ]

  gnome.generate_gir(libemeus,
                     sources: public_headers + sources,
                     namespace: 'Emeus',
                     nsversion: emeus_api_version,
                     identifier_prefix: 'Emeus',
                     symbol_prefix: 'emeus',
                     export_packages: 'emeus-@0@'.format(emeus_api_version),
                     includes: [ 'Gtk-3.0' ],
                     install: true,
                     extra_args: gir_extra_args)
endif

subdir('tests')
