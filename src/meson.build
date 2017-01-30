public_headers = [
  'emeus-constraint.h',
  'emeus-constraint-layout.h',

  'emeus-version-macros.h',
  'emeus-types.h',
  'emeus-utils.h',
]

private_headers = [
  'emeus-constraint-private.h',
  'emeus-constraint-layout-private.h',
  'emeus-expression-private.h',
  'emeus-macros-private.h',
  'emeus-simplex-solver-private.h',
  'emeus-types-private.h',
  'emeus-utils-private.h',
  'emeus-vfl-parser-private.h',
]

solver_sources = [
  'emeus-expression.c',
  'emeus-simplex-solver.c',
  'emeus-utils.c',
  'emeus-vfl-parser.c',
]

sources = [
  'emeus-constraint.c',
  'emeus-constraint-layout.c',
  'emeus-types.c',
]

# Version macros
version_conf = configuration_data()
version_conf.set('EMEUS_MAJOR_VERSION', emeus_major_version)
version_conf.set('EMEUS_MINOR_VERSION', emeus_minor_version)
version_conf.set('EMEUS_MICRO_VERSION', emeus_micro_version)

# Generated headers
configure_file(output: 'config.h',
               configuration: conf)
configure_file(input: 'emeus-version.h.meson',
               output: 'emeus-version.h',
               configuration: version_conf,
               install: true,
               install_dir: 'include/emeus-1.0')

install_headers(public_headers + [ 'emeus.h' ], subdir: 'emeus-1.0')

libemeus_private = static_library('emeus-private',
                                  sources: solver_sources + private_headers,
                                  dependencies: [ mathlib_dep, glib_dep ],
                                  c_args: common_cflags + debug_cflags + [
                                    '-DEMEUS_COMPILATION=1',
                                    '-DG_LOG_USE_STRUCTURED=1',
                                    '-DG_LOG_DOMAIN="Emeus"',
                                  ],
                                  link_args: [ '-Wl,-Bsymbolic-functions' ],
                                  install: false)

libemeus = shared_library('emeus-@0@'.format(emeus_api_version),
                          sources: sources + solver_sources + private_headers,
                          soversion: 0,
                          version: libversion,
                          install: true,
                          dependencies: [ gtk_dep, mathlib_dep ],
                          c_args: common_cflags + debug_cflags + gtk_version_cflags + [
                            '-DEMEUS_COMPILATION=1',
                            '-DG_LOG_USE_STRUCTURED=1',
                            '-DG_LOG_DOMAIN="Emeus"',
                          ],
                          link_with: libemeus_private,
                          link_args: [ '-Wl,-Bsymbolic-functions' ])

# Internal dependency, for tests
emeus_inc = include_directories('.')
emeus_dep = declare_dependency(link_with: libemeus,
                               include_directories: [ emeus_inc ],
                               dependencies: [ gtk_dep, mathlib_dep ])

# PkgConfig file
pkgconf = configuration_data()
pkgconf.set('prefix', emeus_prefix)
pkgconf.set('exec_prefix', emeus_prefix)
pkgconf.set('libdir', emeus_libdir)
pkgconf.set('includedir', emeus_includedir)
pkgconf.set('EMEUS_API_VERSION', emeus_api_version)
pkgconf.set('EMEUS_VERSION', meson.project_version())
pkgconf.set('EMEUS_REQUIRES', 'gtk+-3.0 >= ' + gtk_version_required)

configure_file(input: 'emeus.pc.in',
               output: 'emeus-@0@.pc'.format(emeus_api_version),
               configuration: pkgconf,
               install: true,
               install_dir: join_paths(emeus_libdir, 'pkgconfig'))

# Introspection
if get_option('enable-introspection')
  gir_extra_args = [
    '--c-include=emeus.h',
    '-DEMEUS_COMPILATION',
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
subdir('tools')
