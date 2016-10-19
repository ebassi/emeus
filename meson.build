project('emeus', 'c',
        version: '1.0.0',
        default_options: [
          'buildtype=debug',
          'c_std=c99',
          'warning_level=1'
        ],
        license: 'LGPLv2.1+',
        meson_version: '>= 0.34.0')

emeus_version = meson.project_version().split('.')
emeus_major_version = emeus_version[0].to_int()
emeus_minor_version = emeus_version[1].to_int()
emeus_micro_version = emeus_version[2].to_int()

emeus_api_version = '@0@.0'.format(emeus_major_version)

emeus_interface_age = 0
emeus_binary_age = emeus_minor_version * 100 + emeus_micro_version

emeus_is_development = emeus_minor_version.is_odd()

emeus_prefix = get_option('prefix')
emeus_libdir = emeus_prefix + '/' + get_option('libdir')
emeus_datadir = emeus_prefix + '/share'
emeus_includedir = emeus_prefix + '/include'

cc = meson.get_compiler('c')
host_system = host_machine.system()

conf = configuration_data()

# Compat variables for pkgconfig
conf.set('prefix', emeus_prefix)
conf.set('exec_prefix', emeus_prefix)
conf.set('libdir', emeus_libdir)
conf.set('includedir', emeus_includedir)
conf.set('EMEUS_API_VERSION', emeus_api_version)
conf.set('EMEUS_VERSION', meson.project_version())

# Compiler flags
test_cflags = [
  '-fstrict-aliasing',
  '-Wpointer-arith',
  '-Wmissing-declarations',
  '-Wformat=2',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Wold-style-definition',
  '-Wunused',
  '-Wuninitialized',
  '-Wshadow',
  '-Wmissing-noreturn',
  '-Wmissing-format-attribute',
  '-Wredundant-decls',
  '-Wlogical-op',
  '-Wcast-align',
  '-Wno-unused-local-typedefs',
  '-Werror=implicit',
  '-Werror=init-self',
  '-Werror=main',
  '-Werror=missing-braces',
  '-Werror=return-type',
  '-Werror=array-bounds',
  '-Werror=write-strings'
]
common_cflags = []
foreach cflag: test_cflags
  if cc.has_argument(cflag)
    common_cflags += [ cflag ]
  endif
endforeach

# Check for headers
conf.set('HAVE_STDLIB_H', cc.has_header('stdlib.h'))
conf.set('HAVE_STDINT_H', cc.has_header('stdint.h'))
conf.set('HAVE_STDBOOL_H', cc.has_header('stdbool.h'))

# Version macros
version_conf = configuration_data()
version_conf.set('EMEUS_MAJOR_VERSION', emeus_major_version)
version_conf.set('EMEUS_MINOR_VERSION', emeus_minor_version)
version_conf.set('EMEUS_MICRO_VERSION', emeus_micro_version)

# Debugging
debug_cflags = []
enable_debug = get_option('enable-debug')
if enable_debug == 'auto'
  if emeus_is_development
    enable_debug = 'yes'
  else
    enable_debug = 'minimum'
  endif
endif

if enable_debug == 'yes'
  debug_cflags += [
    '-DEMEUS_ENABLE_DEBUG'
  ]
endif

if enable_debug == 'minimum'
  debug_cflags += [
    '-DEMEUS_ENABLE_DEBUG',
    '-DG_DISABLE_CAST_CHECKS'
  ]
endif

if enable_debug == 'no'
  debug_cflags += [
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_CHECKS',
    '-DG_DISABLE_ASSERT'
  ]
endif

# Detect and set symbol visibility
if get_option('default_library') != 'static'
  if host_system == 'windows'
    conf.set('DLL_EXPORT', true)
    if cc.get_id() == 'msvc'
      conf.set('_EMEUS_PUBLIC', '__declspec(dllexport) extern')
    else
      conf.set('_EMEUS_PUBLIC', '__attribute__((visibility("default"))) __declspec(dllexport) extern')
      add_global_arguments('-fvisibility=hidden', language: 'c')
    endif
  else
    conf.set('_EMEUS_PUBLIC', '__attribute__((visibility("default")))')
    add_global_arguments('-fvisibility=hidden', language: 'c')
  endif
endif

# Required dependencies
mathlib_dep = cc.find_library('m', required: true)

glib_dep = dependency('glib-2.0', version: '>= 2.46', required: true)

gtk_version_req_major = 3
gtk_version_req_minor = 20
gtk_version_req_micro = 0
gtk_version_required = '@0@.@1@.@2@'.format(gtk_version_req_major, gtk_version_req_minor, gtk_version_req_micro)
gtk_version_is_development = gtk_version_req_minor.is_odd()
gtk_version_cflags = []
if gtk_version_is_development
  # Ensure we're using the newly added API at most
  min_required = 'GDK_VERSION_@0@_@1@'.format(gtk_version_req_major, gtk_version_req_minor - 1)
  max_allowed = 'GDK_VERSION_@0@_@1@'.format(gtk_version_req_major, gtk_version_req_minor + 1)
  gtk_version_cflags += [
    '-DGDK_VERSION_MIN_REQUIRED=' + min_required,
    '-DGDK_VERSION_MAX_ALLOWED=' + max_allowed,
  ]
endif

gtk_dep = dependency('gtk+-3.0', version: '>= ' + gtk_version_required, required: true)

gnome = import('gnome')

subdir('src')
subdir('doc')
subdir('examples')
