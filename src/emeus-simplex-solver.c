#include "config.h"

#include "emeus-simplex-solver-private.h"

#include "emeus-types-private.h"
#include "emeus-variable-private.h"
#include "emeus-expression-private.h"

SimplexSolver *
simplex_solver_new (void)
{
  SimplexSolver *res = g_slice_new0 (SimplexSolver);

  return res;
}
