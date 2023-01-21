project('emeus', 'c',
        version: '0.99.1',
        default_options: [
          'buildtype=debugoptimized',
          'c_std=c99',
          'warning_level=1'
        ],
        license: 'LGPLv2.1+',
        meson_version: '>= 0.59')

emeus_version = meson.project_version().split('.')
emeus_major_version = emeus_version[0].to_int()
emeus_minor_version = emeus_version[1].to_int()
emeus_micro_version = emeus_version[2].to_int()

emeus_is_development = emeus_minor_version.is_odd()

if emeus_minor_version > 90
  emeus_api_version = '@0@.0'.format(emeus_major_version + 1)
else
  emeus_api_version = '@0@.0'.format(emeus_major_version)
endif

if emeus_is_development
  emeus_interface_age = 0
else
  emeus_interface_age = emeus_micro_version
endif

emeus_binary_age = 100* emeus_minor_version + emeus_micro_version

# Shared library versioning
soversion = 0
libversion = '@0@.@0@.@1@'.format(soversion, emeus_binary_age - emeus_interface_age, emeus_interface_age)

# Paths for the pkg-config file
emeus_prefix = get_option('prefix')
emeus_bindir = join_paths(emeus_prefix, get_option('bindir'))
emeus_libdir = join_paths(emeus_prefix, get_option('libdir'))
emeus_datadir = join_paths(emeus_prefix, get_option('datadir'))
emeus_includedir = join_paths(emeus_prefix, get_option('includedir'))
emeus_libexecdir = join_paths(emeus_prefix, get_option('libexecdir'))
emeus_localedir = join_paths(emeus_datadir, 'locale')

emeus_api_path = '@0@-@1@'.format(meson.project_name(), emeus_api_version)

cc = meson.get_compiler('c')
host_system = host_machine.system()

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(meson.project_name(), meson.project_version()))
conf.set_quoted('PACKAGE_DATADIR', emeus_datadir)
conf.set_quoted('PACKAGE_LIBDIR', emeus_libdir)
conf.set_quoted('PACKAGE_LOCALE_DIR', emeus_localedir)
conf.set_quoted('PACKAGE_LIBEXECDIR', emeus_libexecdir)
conf.set('GETTEXT_PACKAGE', 'PACKAGE_NAME')
conf.set('LOCALEDIR', 'PACKAGE_LOCALE_DIR')
conf.set10('ENABLE_NLS', true) # Always enabled

# Compiler flags
test_cflags = [
  '-fstrict-aliasing',
  '-Wpointer-arith',
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wmissing-format-attribute',
  '-Wmissing-prototypes',
  '-Wformat=2',
  '-Wstrict-prototypes',
  '-Wnested-externs',
  '-Wold-style-definition',
  '-Wunused',
  '-Wuninitialized',
  '-Wshadow',
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
    common_cflags += cflag
  endif
endforeach

common_ldflags = []
if host_system == 'linux'
  foreach ldflag: [ '-Wl,-Bsymbolic-functions', '-Wl,-z,relro', '-Wl,-z,now', ]
    common_ldflags += ldflag
  endforeach
endif

# Check for headers
conf.set('HAVE_STDLIB_H', cc.has_header('stdlib.h'))
conf.set('HAVE_STDINT_H', cc.has_header('stdint.h'))
conf.set('HAVE_STDBOOL_H', cc.has_header('stdbool.h'))

# Debugging
debug_cflags = []
buildtype = get_option('buildtype')
if buildtype.startswith('debug')
  debug_cflags += '-DEMEUS_ENABLE_DEBUG'
elif buildtype == 'release'
  debug_cflags += [
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_CHECKS',
    '-DG_DISABLE_ASSERT',
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
      common_ldflags += '-fvisibility=hidden'
    endif
  else
    conf.set('_EMEUS_PUBLIC', '__attribute__((visibility("default")))')
    common_ldflags += '-fvisibility=hidden'
  endif
endif

# Required dependencies
mathlib_dep = cc.find_library('m', required: false)

glib_dep = dependency('glib-2.0', version: '>= 2.46')

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

gtk_dep = dependency('gtk+-3.0', version: '>= ' + gtk_version_required)

i18n = import('i18n')
gnome = import('gnome')

subdir('src')
subdir('examples')

subdir('po')

if get_option('docs')
  subdir('doc')
endif

gnome = import('gnome')
update_desktop_database = find_program('update-desktop-database', required: false)
gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: update_desktop_database.found(),
)
