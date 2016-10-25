e = executable('solver', 'solver.c',
               include_directories: emeus_inc,
               dependencies: [ glib_dep, mathlib_dep ],
               objects: solver_objects)
test('Solver', e)

e = executable('vfl-parser', 'vfl-parser.c',
               include_directories: emeus_inc,
               dependencies: [ glib_dep, mathlib_dep ],
               objects: solver_objects)
test('VflParser', e)
