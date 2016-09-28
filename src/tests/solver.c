#include <glib.h>
#include <math.h>
#include <string.h>

#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-types-private.h"
#include "emeus-variable-private.h"

static void
emeus_solver_basic (void)
{
  SimplexSolver solver;

  simplex_solver_init (&solver);

  simplex_solver_clear (&solver);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/emeus/solver/basic", emeus_solver_basic);

  return g_test_run ();
}
