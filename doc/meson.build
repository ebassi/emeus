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

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
gtk_prefix = dependency('gtk+-3.0').get_pkgconfig_variable('prefix')
gtk_docpath = join_paths(gtk_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(emeus_datadir, 'gtk-doc', 'html')

gnome.gtkdoc('emeus',
             main_xml: 'emeus-docs.xml',
             src_dir: join_paths(meson.source_root(), 'src'),
             dependencies: emeus_dep,
             gobject_typesfile: 'emeus.types',
             scan_args: [
               '--rebuild-types',
               '--ignore-decorators=_EMEUS_PUBLIC',
               '--ignore-headers=' + ' '.join(ignore_headers),
             ],
             fixxref_args: [
               '--html-dir=@0@'.format(docpath),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
               '--extra-dir=@0@'.format(join_paths(gtk_docpath, 'gtk3')),
             ],
             install: true)

xsltproc = find_program('xsltproc', required: false)

if xsltproc.found()
  xlstproc_flags = [
    '--nonet',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'man.th.extra1.suppress', '1',
    '--stringparam', 'man.authors.section.enabled', '1',
    '--stringparam', 'man.copyright.section.enabled', '1',
  ]

  man_input_files = [
    'emeus-gen-constraints.xml',
  ]

  man_output_files = [
    'emeus-gen-constraints.1',
  ]

  man_target = custom_target('man pages',
                             input: man_input_files,
                             output: man_output_files,
                             command: [
                               xsltproc,
                               xlstproc_flags,
                               '-o', '@OUTPUT@',
                               'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
                               '@INPUT@',
                             ],
                             install: true,
                             install_dir: join_paths(get_option('mandir'), 'man1'))
endif
