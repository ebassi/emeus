#include <glib.h>
#include <math.h>
#include <string.h>

#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-types-private.h"
#include "emeus-variable-private.h"

#define emeus_fuzzy_equals(n1,n2,epsilon) \
  (((n1) > (n2) ? ((n1) - (n2)) : ((n2) - (n1))) < (epsilon))

#define emeus_assert_fuzzy_equals(n1,n2,epsilon) \
  G_STMT_START { \
    if (emeus_fuzzy_equals (n1, n2, epsilon)) ; else { \
      g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                  #n1 " == " #n2 " (+/- " #epsilon ")", \
                                  n1, "==", n2, 'f'); \
    } \
  } G_STMT_END

static void
emeus_solver_simple (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver);
  variable_set_value (x, 167.0);
  variable_set_name (x, "x");

  Variable *y = simplex_solver_create_variable (&solver);
  variable_set_value (y, 2.0);
  variable_set_name (y, "y");

  Expression *e = expression_new_from_variable (y);

  simplex_solver_add_constraint (&solver,
                                 x, OPERATOR_TYPE_EQ, e,
                                 STRENGTH_REQUIRED);

  double x_value = variable_get_value (x);
  double y_value = variable_get_value (y);

  emeus_assert_fuzzy_equals (x_value, y_value, 1e-8);
  emeus_assert_fuzzy_equals (x_value, 0.0, 1e-8);
  emeus_assert_fuzzy_equals (y_value, 0.0, 1e-8);

  expression_unref (e);
  variable_unref (y);
  variable_unref (x);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_stay (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver);
  variable_set_value (x, 5.0);
  variable_set_name (x, "x");

  Variable *y = simplex_solver_create_variable (&solver);
  variable_set_value (y, 10.0);
  variable_set_name (y, "y");

  simplex_solver_add_stay_variable (&solver, x, STRENGTH_WEAK);
  simplex_solver_add_stay_variable (&solver, y, STRENGTH_WEAK);

  double x_value = variable_get_value (x);
  double y_value = variable_get_value (y);

  emeus_assert_fuzzy_equals (x_value, 5.0, 1e-8);
  emeus_assert_fuzzy_equals (y_value, 10.0, 1e-8);

  simplex_solver_clear (&solver);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/emeus/solver/simple", emeus_solver_simple);
  g_test_add_func ("/emeus/solver/stay", emeus_solver_stay);

  return g_test_run ();
}
