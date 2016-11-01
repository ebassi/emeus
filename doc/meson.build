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
]

if get_option('enable-gtk-doc')
  gnome.gtkdoc('emeus',
               main_xml: 'emeus-docs.xml',
               src_dir: meson.source_root() + '/src',
               scan_args: [
                 '--rebuild-types',
                 '--ignore-decorators=_EMEUS_PUBLIC',
                 '--ignore-headers=' + ' '.join(ignore_headers),
               ],
               install: true)
endif
