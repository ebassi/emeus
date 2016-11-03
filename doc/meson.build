subdir('xml')

ignore_headers = [
  'emeus.h',
  'config.h',
  'emeus-constraint-layout-private.h',
  'emeus-constraint-private.h',
  'emeus-expression-private.h',
  'emeus-macros-private.h',
  'emeus-simplex-solver-private.h',
  'emeus-types-private.h',
  'emeus-utils-private.h',
  'emeus-variable-private.h',
  'emeus-vfl-parser-private.h',
  'emeus-test-utils.h',
]

extra_dir_arg = '--extra-dir=' + get_option('prefix') + '/share/gtk-doc/html'

gnome.gtkdoc('emeus',
             main_xml: 'emeus-docs.xml',
             src_dir: meson.source_root() + '/src',
             dependencies: emeus_dep,
             gobject_typesfile: 'emeus.types',
             scan_args: [
               '--rebuild-types',
               '--ignore-decorators=_EMEUS_PUBLIC',
               '--ignore-headers=' + ' '.join(ignore_headers),
             ],
             fixxref_args: [
               '--html-dir=@0@/@1@/gtk-doc/html'.format(get_option('prefix'), get_option('datadir')),
               extra_dir_arg + '/glib',
               extra_dir_arg + '/gobject',
               extra_dir_arg + '/gtk',
             ],
             install: true)
