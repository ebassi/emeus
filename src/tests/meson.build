solver_sources = [
  'emeus-expression.c',
  'emeus-simplex-solver.c',
  'emeus-utils.c',
  'emeus-variable.c',
]

solver_objects = libemeus.extract_objects(solver_sources)

e = executable('solver', 'solver.c',
               include_directories: emeus_inc,
               dependencies: [ glib_dep, mathlib_dep ],
               objects: solver_objects)
test('Solver', e)
