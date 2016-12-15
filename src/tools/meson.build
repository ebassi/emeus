msgfmt = find_program('msgfmt')
podir = join_paths(meson.source_root(), 'po')

gen_constraints = executable('emeus-gen-constraints', 'emeus-gen-constraints.c',
                             include_directories: emeus_inc,
                             dependencies: [ glib_dep, mathlib_dep ],
                             link_with: libemeus_private,
                             install: true)

# Desktop launcher and description file.
custom_target('desktop-file',
              input: 'com.endlessm.EmeusEditor.desktop.in.in',
              output: 'com.endlessm.EmeusEditor.desktop',
              install: true,
              install_dir: join_paths(get_option('datadir'), 'applications'),
              command: [
                msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'
              ])

# Service file
serviceconf = configuration_data()
serviceconf.set('bindir', emeus_bindir)

configure_file(input: 'com.endlessm.EmeusEditor.service.in',
               output: 'com.endlessm.EmeusEditor.service',
               configuration: serviceconf,
               install: true,
               install_dir: join_paths(get_option('datadir'), 'dbus-1/services'))

editor_sources = [
  'emeus-edit-constraints.c'
]

editor_resources = gnome.compile_resources('editor-resources',
                                           'editor.gresource.xml',
                                           c_name: '_editor')

editor = executable('emeus-edit-constraints',
                    editor_sources + editor_resources,
                    include_directories: emeus_inc,
                    c_args: [ '-DG_LOG_STRUCTURED=1', '-DG_LOG_DOMAIN="Editor"' ],
                    dependencies: [ glib_dep, gtk_dep, emeus_dep ],
                    link_with: libemeus_private,
                    install: true)
