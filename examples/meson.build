examples = [
  'simple-grid.c',
  'simple-grid.js',
  'centered.js',
]

examples_dir = get_option('datadir') + '/emeus-@0@/examples'.format(emeus_api_version)

install_data(examples, install_dir: examples_dir)

executable('simple-grid', 'simple-grid.c',
           dependencies: [ gtk_dep, emeus_dep ],
           install: true,
           install_dir: examples_dir)
