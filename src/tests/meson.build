tests = [
  [ 'solver', 'solver.c' ],
  [ 'vfl-parser', 'vfl-parser.c' ],
]

foreach t: tests
  e = executable(t[0], t[1],
                 include_directories: emeus_inc,
                 dependencies: [ glib_dep, mathlib_dep ],
                 objects: solver_objects)
  test(t[0], e)
endforeach
